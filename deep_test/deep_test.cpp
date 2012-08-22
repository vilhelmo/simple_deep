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

bool writeImageFile(std::string filename, int xres, int yres, int channels, float * data) {
	/*
	 * WARNING: This makes the assumption that the folder to write the image exists.
	 * It will crash if the filename is pointing to a folder that doesn't exist.
	 */
	OpenImageIO::ImageOutput * out = OpenImageIO::ImageOutput::create(filename);
	if (!out) {
		return false;
	}
	OpenImageIO::ImageSpec spec(xres, yres, channels, OpenImageIO::TypeDesc::FLOAT);
	out->open(filename.c_str(), spec);
	out->write_image(OpenImageIO::TypeDesc::FLOAT, data);
	out->close();
	delete out;
	return true;
}

void drawCircle(float cx, float cy, float r, std::initializer_list<float> color, deep::Image * img) {
	float r2 = pow(r, 2.0);
	float xyFactor2 = pow(float(img->width())/float(img->height()), 2.0);
	int superSamplingFactor = 1;
	int w4 = superSamplingFactor*img->width();
	int h4 = superSamplingFactor*img->height();
	std::vector<float> c(4);
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

void drawDeepCircle(float z, int cx, int cy, int r, std::initializer_list<float> color, deep::DeepImage * img) {
	float r2 = pow(r, 2.0);
	float xyFactor2 = pow(float(img->width())/float(img->height()), 2.0);
	std::vector<float> c(4);
	int i = 0;
	for (auto iter = color.begin(); iter != color.end(); ++iter) {
		c[i] = (*iter);
		++i;
	}
	for (int y = 0; y < img->height(); ++y) {
		float fy = float(y)/(img->height()-1.0);
		for (int x = 0; x < img->width(); ++x) {
			// (x-x0)^2 + (y-y0^2) <= r^2
			if (r2 - pow(float(x)-float(cx), 2.0) - pow(float(y)-float(cy), 2.0) >= 0.0) { ///xyFactor2
				float fx = float(x)/(img->width()-1.0);
				img->addSample(z, fy, fx, c);
			}
		}
	}
}

void drawDeepRect(float z, int x1, int y1, int x2, int y2, std::initializer_list<float> color, deep::DeepImage * img) {
	std::vector<float> c(4);
	int i = 0;
	for (auto iter = color.begin(); iter != color.end(); ++iter) {
		c[i] = (*iter);
		++i;
	}
	for (int y = y1; y < y2; ++y) {
		float fy = float(y)/float(img->height()-1.0);
		for (int x = x1; x < x2; ++x) {
			float fx = float(x)/float(img->width()-1.0);
			img->addSample(z, fy, fx, c);
		}
	}
}

int main() {
	int scale = 1;
	int x = 640*scale;
	int y = 480*scale;
	std::vector<std::string> c = {"R", "G", "B", deep::ALPHA};

//	deep::Image * fimg = new deep::Image(x, y, c, "Gaussian 5");
//	drawCircle(0.3, 0.4, 0.12, {0.96f, 0.65f, 0.1f, 0.5f}, fimg);
//	drawCircle(0.4, 0.6, 0.17, {0.65f, 0.96f, 0.1f, 0.7f}, fimg);
//	drawCircle(0.9, 0.8, 0.17, {0.05f, 0.06f, 0.99f, 1.f}, fimg);
////	img->addSample(0.302f, 0.400009f, {0.96f, 0.65f, 0.1f});
//	writeImageFile("images/foo.png", x, y, 3, fimg->data(0,0,0));
//	delete fimg;

	deep::DeepImage * img = new deep::DeepImage(x, y, c);

	drawDeepCircle(5.0f, 400*scale, 300*scale, 100*scale, {1.f, 0.f, 0.f, 0.6f}, img);
	drawDeepCircle(1.0f, 300*scale, 200*scale, 100*scale, {0.f, 1.f, 0.f, 0.6f}, img);
	drawDeepCircle(3.0f, 275*scale, 300*scale, 100*scale, {0.f, 0.f, 1.f, 0.6f}, img);
	drawDeepRect(4.0f, 100*scale, 200*scale, 250*scale, 350*scale, {1.f, 1.f, 1.f, 0.6f}, img);
	drawDeepRect(1.0f, 50*scale, 250*scale, 150*scale, 400*scale, {1.f, 0.f, 1.f, 1.0f}, img);

	deep::Image * flatImg = deep::renderDeepImage(*img);
	writeImageFile("deep2flat.png", x, y, 4, flatImg->data(0,0,0));

	printDeepImageStats(*img);

	deep::DeepImageWriter writer("deep.sdf", *img);
	writer.open();
	writer.write();
	writer.close();
	delete img;
	delete flatImg;

	deep::DeepImageReader reader("deep.sdf");
	deep::DeepImage * np = reader.read();

	printDeepImageStats(*np);

	flatImg = deep::renderDeepImage(*np);
	writeImageFile("deep_read2flat.png", x, y, 4, flatImg->data(0,0,0));

	delete np;
	delete flatImg;
}
