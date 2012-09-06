/*
 * image.cpp
 *
 *  Created on: Aug 21, 2012
 *      Author: vilhelm
 */

#include "image.h"
#include "deep.h"
#include "filter.h"

namespace deep {


Image::Image(int inWidth, int inHeight, std::vector<std::string> inChannelNames, std::string pixelFilter) :
		mWidth(inWidth), mHeight(inHeight), mChannelNames(inChannelNames), mFilter(nullptr) {
//	std::cout << "Ctr Image" << std::endl;
//	for (auto cn : mChannelNames) {
//		std::cout << cn << " ";
//	}
//	std::cout << std::endl;

	mData = new ImageDataType[width()*height()*channels()];
	std::istringstream iss(pixelFilter);
	std::string type;
	iss >> type;
	if (type.compare("Nearest") == 0) {
		mFilter = new NNFilter();
//		std::cout << "Using Nearest" << std::endl;
	} else if (type.compare("Linear") == 0) {
		mFilter = new LinearFilter();
//		std::cout << "Using Linear" << std::endl;
	} else { // type == "Gaussian"
		int width = 2;
		iss >> width;
		mFilter = new GaussianFilter(width);
//		std::cout << "Using Gaussian" << std::endl;
	}
}

Image::~Image() {
	delete [] mData;
	mData = nullptr;
	delete mFilter;
	mFilter = nullptr;
}

ImageDataType * Image::data(int y, int x, int c) {
	if (0 < x < width() && 0 < y < height()) {
		return &mData[(y*width() + x)*channels() + c];
	} else {
		return nullptr;
	}
}

ImageDataType Image::data(int y, int x, int c) const {
	if (0 < x < width() && 0 < y < height()) {
		return mData[(y*width() + x)*channels() + c];
	} else {
		return 0.0;
	}
}

void Image::addSample(float y, float x, std::initializer_list<ImageDataType> list) {
	std::vector<float> c(4);
	int i = 0;
	for (auto iter = list.begin(); iter != list.end(); ++iter) {
		c[i] = (*iter);
		++i;
	}
	addSample(y, x, c);
}

void Image::addSample(float y, float x, std::vector<ImageDataType> list) {
	float fy = y * float(height());
	float fx = x * float(width());
	int irx = std::max(std::min(mFilter->minX(fx), width() - 1), 0);
	int ry = std::max(std::min(mFilter->minY(fy), height() - 1), 0);
	int mx = std::max(std::min(mFilter->maxX(fx), width() - 1), 0);
	int my = std::max(std::min(mFilter->maxY(fy), height() - 1), 0);
	for (; ry <= my; ++ry) {
		for (int rx=irx; rx <= mx; ++rx) {
			float gx = mFilter->filter(fx, fy, rx, ry);
			ImageDataType * dataPtr = data(ry, rx, 0);
			for (auto iter = list.begin(); iter != list.end(); ++iter) {
				*dataPtr += (*iter)*gx;
				dataPtr++;
			}
		}
	}
}


} // End namespace
