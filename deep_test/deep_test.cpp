/*
 * deep_test.cpp
 *
 *  Created on: Aug 1, 2012
 *      Author: vilhelm
 */

#include <string>
#include <vector>
#include <math.h>
#include <limits.h>
#include <OpenImageIO/imageio.h>
#include <deep.h>
#include <image.h>
#include <deepimage.h>
#include <deepio.h>

bool writeImageFile(std::string filename, int xres, int yres, int channels, deep::ImageDataType * data) {
	/*
	 * WARNING: This makes the assumption that the folder to write the image exists.
	 * It will crash if the filename is pointing to a folder that doesn't exist.
	 */
	OpenImageIO::ImageOutput * out = OpenImageIO::ImageOutput::create(filename);
	if (!out) {
		return false;
	}
	OpenImageIO::ImageSpec spec(xres, yres, channels, OpenImageIO::TypeDesc::DOUBLE); //FLOAT);
	out->open(filename.c_str(), spec);
	out->write_image(OpenImageIO::TypeDesc::DOUBLE, data); //FLOAT, data);
	out->close();
	delete out;
	return true;
}

void drawCircle(float cx, float cy, float r, std::initializer_list<deep::ImageDataType> color, deep::Image * img) {
	float r2 = pow(r, 2.0);
	float xyFactor2 = pow(float(img->width())/float(img->height()), 2.0);
	int superSamplingFactor = 1;
	int w4 = superSamplingFactor*img->width();
	int h4 = superSamplingFactor*img->height();
	std::vector<deep::ImageDataType> c(4);
	int i = 0;
	for (auto iter = color.begin(); iter != color.end(); ++iter) {
		c[i] = (*iter) / float(superSamplingFactor*superSamplingFactor);
		++i;
	}
	for (int y = 0; y < h4; ++y) {
		float fy = float(y)/(h4-1.0);
		for (int x = 0; x < w4; ++x) {
			float fx = float(x)/(w4-1.0);
			// (x-x0)^2 + (y-y0^2) <= r^2
			if (r2 - pow(fx-cx, 2.0) - pow(fy-cy, 2.0)/xyFactor2 >= 0.0) {
				img->addSample(fy, fx, c);
			}
		}
	}
}

void drawDeepCircle(int cx, int cy, int r, std::initializer_list<deep::DeepDataType> color, deep::DeepImage * img) {
	float r2 = pow(r, 2.0);
	float xyFactor2 = pow(float(img->width())/float(img->height()), 2.0);
	std::vector<deep::DeepDataType> c(color.size());
	int i = 0;
	for (auto iter = color.begin(); iter != color.end(); ++iter) {
		c[i] = (*iter);
		++i;
	}
	for (int y = 0; y < img->height(); ++y) {
		float fy = float(y)/(img->height()-1.0);
		for (int x = 0; x < img->width(); ++x) {
			// (x-x0)^2 + (y-y0^2) <= r^2
			if (r2 - pow(float(x)-float(cx), 2.0) - pow(float(y)-float(cy), 2.0) >= 0.0) {
				float fx = float(x)/(img->width()-1.0);
				img->addSample(y, x, c);
			}
		}
	}
}

void drawDeepRect(int x1, int y1, int x2, int y2, std::initializer_list<deep::DeepDataType> color, deep::DeepImage * img) {
	std::vector<deep::DeepDataType> c(color.size());
	int i = 0;
	for (auto iter = color.begin(); iter != color.end(); ++iter) {
		c[i] = (*iter);
		++i;
	}
	for (int y = y1; y < y2; ++y) {
		float fy = float(y)/float(img->height()-1.0);
		for (int x = x1; x < x2; ++x) {
			float fx = float(x)/float(img->width()-1.0);
			img->addSample(y, x, c);
		}
	}
}

float evalFunc(const std::array<float, 3> & f, float x) {
	if (x <= f[0]) {
		return 1.f;
	} else if (x >= f[1]) {
		return f[2];
	} else {
		// lin interp
		return 1.f - (x - f[0])*(1.f - f[2])/(f[1] - f[0]);
	}
}

void testTransFunction() {
	std::map<float, std::array<float, 2>> test;

	std::vector<std::array<float, 3>> funcs;
	funcs.push_back({{0.2, 0.2, 0.5}});
	funcs.push_back({{0.5, 0.8, 0.5}});
	funcs.push_back({{1.0, 1.0, 0.5}});
	funcs.push_back({{0.6, 0.6, 0.5}});

	for (auto & f : funcs) {
		std::array<float, 2> & t = test[f[0]];
		t[0] = 1.f;
		if (f[1] > f[0]) {
			if (std::fabs(t[1]) < 0.0000001) {
				t[1] = 1.f;
			}
			std::array<float, 2> & t2 = test[f[1]];
			t2[0] = 1.f;
			if (std::fabs(t2[1]) < 0.0000001) {
				t2[1] = 1.f;
			}
		} else {
			t[1] = -1.f;
		}
	}

	for (auto a : test) {
		std::cout << a.first << " " << a.second[0] << " " << a.second[1] << std::endl;
	}

	for (auto & f : funcs) {
		auto iter = test.find(f[0]);
		if (iter != test.end()) {
			std::array<float, 2> & ys = iter->second;
			if (std::fabs(f[1] - f[0]) < 0.0000001) {
				if (ys[1] >= 0.f) {
					ys[1] = ys[1] * f[2];
				} else {
					ys[1] = ys[0] * f[2];
				}
			}
			iter++;
			while (iter != test.end()) {
				float x = iter->first;
				float a = evalFunc(f, x);
				std::array<float, 2> & ys = iter->second;
				ys[0] = ys[0] * a;
				ys[1] = ys[1] * a;
				iter++;
			}
		}
	}
	for (auto a : test) {
		std::cout << a.first << " " << a.second[0] << " " << a.second[1] << std::endl;
	}
}

void testSinglePixelFile(std::vector<std::string> channels, std::string filename) {
	deep::DeepImage * img = new deep::DeepImage(1, 1, channels);
	img->addSample(0, 0, {1.f, 1.f, 0.f, 0.5f, 1.f, 1.f});
	img->addSample(0, 0, {0.f, 1.f, 1.f, 0.5f, 4.f, 4.f});
//	img->addSample(0, 0, {1.f, 1.f, 0.f, 1.f, 5.f, 5.f});
	deep::Image * flatImg = deep::renderDeepImage(*img);
	writeImageFile(filename, 1, 1, flatImg->channels(), flatImg->data(0,0,0));
	int c = 0;
	for (auto channelName : flatImg->channelNames()) {
		std::cout << channelName << " : " << *flatImg->data(0,0,c) << std::endl;
		c++;
	}
	delete img;
	delete flatImg;
}

void testFlatFile1(std::string filename) {
	int x = 640;
	int y = 480;
	std::vector<std::string> c = {"R", "G", "B", deep::ALPHA};
	deep::Image * img = new deep::Image(x, y, c, "Nearest");
	drawCircle(0.6, 0.6, 0.2, {0.f, 0.f, 1.f, 0.5f}, img);
	drawCircle(0.4, 0.4, 0.2, {0.0, 1.0, 0.0, 0.25f}, img);
	writeImageFile(filename, x, y, 4, img->data(0,0,0));
	delete img;
}

void testDeepFile1(std::vector<std::string> channels, std::string filename, std::string deepFilename) {
	int x = 640;
	int y = 480;
	deep::DeepImage * img = new deep::DeepImage(x, y, channels);
//	drawDeepCircle(300, 200, 100, {1.f, 0.f, 0.f, 0.5f, 5.f, 7.f}, img);
	drawDeepCircle(275, 300, 100, {0.f, 1.f, 0.f, 0.5f, 3.f, 6.f}, img);
	drawDeepCircle(400, 300, 100, {0.f, 0.f, 1.f, 0.5f, 2.f, 4.f}, img);
	drawDeepRect(100, 200, 250, 350, {1.f, 1.f, 1.f, 1.f, 1.f, 8.f}, img);
//	drawDeepRect(0, 0, 640, 480, {0.f, 0.f, 0.f, 1.f, 15.f, 15.f}, img);

	std::cout << "Info about " << deepFilename << std::endl;
	printDeepImageStats(*img);

	deep::DeepImageWriter writer(deepFilename, *img);
	writer.open();
	writer.write();
	writer.close();

	deep::Image * flatImg = deep::renderDeepImage(*img);
	delete img;
	writeImageFile(filename, x, y, 4, flatImg->data(0,0,0));
	delete flatImg;
}

void testDeepFile2(std::vector<std::string> channels, std::string filename, std::string deepFilename) {
	int x = 640;
	int y = 480;
	deep::DeepImage * img = new deep::DeepImage(x, y, channels);

	drawDeepCircle(300, 200, 100, {1.f, 0.f, 0.f, 0.5f, 5.f, 7.f}, img);

	std::cout << "Info about " << deepFilename << std::endl;
	printDeepImageStats(*img);

	deep::DeepImageWriter writer(deepFilename, *img);
	writer.open();
	writer.write();
	writer.close();

	deep::Image * flatImg = deep::renderDeepImage(*img);
	delete img;
	writeImageFile(filename, x, y, 4, flatImg->data(0,0,0));
	delete flatImg;
}

void testDeepAddition(std::string deepFilename1, std::string deepFilename2, std::string filenameComb) {
	deep::DeepImageReader reader(deepFilename1);
	deep::DeepImage * d1 = reader.read();
	//		printDeepImageStats(*d1);

	deep::DeepImageReader reader2(deepFilename2);
	deep::DeepImage * d2 = reader2.read();
	//	printDeepImageStats(*d2);

	d1->addDeepImage(*d2);
	//		printDeepImageStats(*d2);

	deep::Image * combImg = deep::renderDeepImage(*d1);
	writeImageFile(filenameComb, combImg->width(), combImg->height(), 4, combImg->data(0,0,0));
	delete d1;
	delete d2;
	delete combImg;
}

void testDeepSubtraction(std::string deepFilename1, std::string deepFilename2, std::string filenameSub) {
	deep::DeepImageReader reader(deepFilename1);
	deep::DeepImage * d1 = reader.read();
//		printDeepImageStats(*d1);

	deep::DeepImageReader reader2(deepFilename2);
	deep::DeepImage * d2 = reader2.read();
//	printDeepImageStats(*d2);

	d1->subtractDeepImage(*d2);
//		printDeepImageStats(*d2);

	deep::Image * subImg = deep::renderDeepImage(*d1);
	std::cout << "Info about " << filenameSub << std::endl;
	deep::printFlatImageStats(*subImg);
	writeImageFile(filenameSub, subImg->width(), subImg->height(), 4, subImg->data(0,0,0));
	delete d1;
	delete d2;
	delete subImg;
}

void testSubAdd(std::string filename1, std::string filename2) {
	deep::DeepImageReader reader(filename1);
	deep::DeepImage * d1 = reader.read();

	deep::DeepImageReader reader2(filename2);
	deep::DeepImage * d2 = reader2.read();

	d1->subtractDeepImage(*d2);
	deep::Image * subImg = deep::renderDeepImage(*d1);
	delete d1;
	d1 = reader.read();

	d2->subtractDeepImage(*d1);
	deep::Image * subImg2 = deep::renderDeepImage(*d2);

	for (int y = 0; y < subImg->height(); ++y) {
		for (int x = 0; x < subImg->width(); ++x) {
			float a1 = *subImg->data(y, x, 3);
			float a2 = *subImg2->data(y, x, 3);
			float a = std::max(std::min(a1 + a2, 1.f), 0.f);
			for (int c = 0; c < subImg->channels()-1; ++c) {
				float d1 = *subImg->data(y, x, c);
				float d2 = *subImg2->data(y, x, c);
				if (d2 < 0.0) {
					std::cerr << "Less than 0 value at (" << x << "," << y << ")[" << c << "]=" << d2 << std::endl;
					throw std::exception();
				} else {
					*subImg->data(y, x, c) = (a1*d1+a2*d2)/a;
				}
			}
			*subImg->data(y, x, 3) = a;
		}
	}

	std::cout << "Info about " << "deepsubcomb.png" << std::endl;
	printFlatImageStats(*subImg);
	writeImageFile("deepsubcomb.png", subImg->width(), subImg->height(), 4, subImg->data(0,0,0));
	delete d2;
	delete d1;
	delete subImg2;
	delete subImg;
}

void testDeepSubtraction2(std::vector<std::string> channels, std::string filenameSub) {
	int x = 640;
	int y = 480;
	deep::DeepImage * img1 = new deep::DeepImage(x, y, channels);
	// Red background.
	drawDeepRect(0, 0, x, y, {1.f, 0.f, 0.f, 1.f, 10.f, 15.f}, img1);

	deep::DeepImage * img2 = new deep::DeepImage(x, y, channels);
	// Green circle.
	drawDeepCircle(400, 300, 100, {0.f, 1.f, 0.f, 0.5f, 5.f, 5.f}, img2);

	img1->subtractDeepImage(*img2);

	deep::Image * subImg = deep::renderDeepImage(*img1);
	writeImageFile(filenameSub, subImg->width(), subImg->height(), 4, subImg->data(0,0,0));

	delete img1;
	delete img2;
}


void testDeepReader(std::string deepFilename, std::string filenameRead) {
	deep::DeepImageReader reader(deepFilename);
	deep::DeepImage * d1 = reader.read();
	printDeepImageStats(*d1);

	deep::Image * img = deep::renderDeepImage(*d1);
	printFlatImageStats(*img);
	writeImageFile(filenameRead, img->width(), img->height(), 4, img->data(0,0,0));
	delete img;
	delete d1;
}


int main() {
	testTransFunction();

	int scale = 1;
	int x = 640*scale;
	int y = 480*scale;
	std::vector<std::string> c = {"R", "G", "B", deep::ALPHA, deep::DEPTH, deep::DEPTH_BACK};

	std::cout << "min type value: " << deep::EPSILON << std::endl;

//	testSinglePixelFile(c, "deep2flat_pixel.png");
//	testFlatFile1("flat.png");
//	testDeepFile1(c, "deep2flat1.png", "deepFile1.sdf");
//	testDeepFile2(c, "deep2flat2.png", "deepFile2.sdf");
//
//	testDeepAddition("deepFile1.sdf", "deepFile2.sdf", "deep12comb.png");
//	testDeepSubtraction("deepFile1.sdf", "deepFile2.sdf", "deep12sub.png");
//	testDeepSubtraction("deepFile2.sdf", "deepFile1.sdf", "deep21sub.png");
//	testSubAdd("deepFile1.sdf", "deepFile2.sdf");
////	testDeepSubtraction2(c, "deep_sub.png");

	testDeepReader("rat2sdf.sdf", "rat2sdf.png");
}
