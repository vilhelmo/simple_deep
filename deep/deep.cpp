/*
 * deep.cpp
 *
 *  Created on: Aug 1, 2012
 *      Author: vilhelm
 */

#include <iostream>
#include "deep.h"
#include "image.h"
#include "deepimage.h"

namespace deep {


void printDeepImageStats(const DeepImage & image) {
	std::cout << "Deep image stats:" << std::endl;
	std::cout << "\twidth: " << image.width() << " height: " << image.height() << std::endl;
	std::cout << "\tnumber of channels: " << image.channels() << std::endl;
	for (auto & name : image.channelNames()) {
		const std::vector<DeepDataType> & data = image.channelData(name);
		DeepDataType min = 10000.0;
		DeepDataType max = -10000.0;
		for (const DeepDataType & d : data) {
			min = std::min(min, d);
			max = std::max(max, d);
		}
		std::cout << "\t" << name << " - " << " min: " << min << " max: " << max << std::endl;
	}
//	std::cout << std::endl;
	std::cout << "\tnumber of elements: " << image.numElements() << ". max number of elements in pixel: " << image.maxElementsInPixel() << std::endl;
}

void printFlatImageStats(const Image & image) {
	std::cout << "Flat image stats:" << std::endl;
	std::cout << "\twidth: " << image.width() << " height: " << image.height() << std::endl;
	std::cout << "\tnumber of channels: " << image.channels() << std::endl;
	int c = 0;
	for (auto & name : image.channelNames()) {
		DeepDataType min = 10000.0;
		DeepDataType max = -10000.0;
		for (int y = 0; y < image.height(); ++y) {
			for (int x = 0; x < image.width(); ++x) {
				float d = image.data(y, x, c);
				min = std::min(min, d);
				max = std::max(max, d);
			}
		}
		c++;
		std::cout << "\t" << name << " - " << " min: " << min << " max: " << max << std::endl;
	}
}

Image * renderDeepImage(const DeepImage & deepImage) {
	Image * renderedImage = new Image(deepImage.width(), deepImage.height(), deepImage.channelNamesNoZ());

	for (int y = 0; y < deepImage.height(); ++y) {
		for (int x = 0; x < deepImage.width(); ++x) {
			ImageDataType * dataPtr = renderedImage->data(y, x, 0);
			std::vector<DeepDataType> pixel;
			if (deepImage.hasZBack()) {
				pixel = deepImage.renderPixelLinear(y, x);
			} else {
				pixel = deepImage.renderPixel(y, x);
			}
			for (auto p : pixel) {
				*dataPtr = p;
				dataPtr++;
			}
		}
	}
	return renderedImage;
}


}
