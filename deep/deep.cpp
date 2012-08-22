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
	std::cout << "width: " << image.width() << " height: " << image.height() << std::endl;
	std::cout << "number of channels: " << image.channels() << std::endl;
	for (auto & name : image.channelNames()) {
		std::cout << name << " ";
	}
	std::cout << std::endl;
	std::cout << "number of elements: " << image.numElements() << ". max number of elements in pixel: " << image.maxElementsInPixel() << std::endl;
}

Image * renderDeepImage(const DeepImage & deepImage) {
	Image * renderedImage = new Image(deepImage.width(), deepImage.height(), deepImage.channelNamesInOrder());
	for (int y = 0; y < deepImage.height(); ++y) {
		for (int x = 0; x < deepImage.width(); ++x) {
			ImageDataType * dataPtr = renderedImage->data(y, x, 0);
			std::vector<DeepDataType> pixel = deepImage.renderPixel(y, x);
			for (auto p : pixel) {
				*dataPtr = p;
				dataPtr++;
			}
		}
	}
	return renderedImage;
}


}
