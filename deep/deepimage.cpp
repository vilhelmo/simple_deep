/*
 * deepimage.cpp
 *
 *  Created on: Aug 21, 2012
 *      Author: vilhelm
 */

#include "deepimage.h"
#include "filter.h"
#include <algorithm>
#include <iterator>

namespace deep {


DeepImage::DeepImage(int inWidth, int inHeight, std::vector<std::string> inChannelNames, std::string pixelFilter) :
		mWidth(inWidth), mHeight(inHeight), mChannelNamesInOrder(inChannelNames), mFilter(nullptr) {
	std::istringstream iss(pixelFilter);
	std::string type;
	iss >> type;
	if (type.compare("Nearest") == 0) {
		mFilter = new NNFilter();
	} else if (type.compare("Linear") == 0) {
		mFilter = new LinearFilter();
	} else { // type == "Gaussian"
		int width = 2;
		iss >> width;
		mFilter = new GaussianFilter(width);
	}

	bool hasZ = false;
	mIndexData = new std::vector<int>[width()*height()];
	for (auto channelName : mChannelNamesInOrder) {
		std::cout << "Creating channel " << channelName << std::endl;
		mChannelData.insert({channelName, std::vector<DeepDataType>()});
		if (channelName.compare(DEPTH) == 0) {
			hasZ = true;
		}
	}
	if (!hasZ) {
		mChannelData.insert({DEPTH, std::vector<DeepDataType>()});
	}
}

DeepImage::~DeepImage() {
	delete mFilter;
	mFilter = nullptr;
	delete [] mIndexData;
	mIndexData = nullptr;
}

void DeepImage::addSample(float z, float y, float x, std::initializer_list<DeepDataType> list) {
	std::vector<DeepDataType> c(list.size());
	int i = 0;
	for (auto iter = list.begin(); iter != list.end(); ++iter) {
		c[i] = (*iter);
		++i;
	}
	addSample(z, y, x, c);
}

void DeepImage::addSample(float z, float y, float x, std::vector<DeepDataType> list) {
	int iy = std::max(std::min(int(y * height()), height() - 1), 0);
	int ix = std::max(std::min(int(x * width()), width() - 1), 0);

	auto channelNameIter = mChannelNamesInOrder.begin();
	for (auto inputIter = list.begin(); inputIter != list.end(); ++inputIter) {
		mChannelData[*channelNameIter].push_back(*inputIter);
		channelNameIter++;
	}
	mChannelData[DEPTH].push_back(z);
	int index = mChannelData[DEPTH].size();
	indexVector(iy, ix)->push_back(index);
}

const std::vector<int> & DeepImage::deepDataIndex(int y, int x) const {
	if (0 < x < width() && 0 < y < height()) {
		return mIndexData[y*width() + x];
	} else {
		// Return something just to be safe.
		return mIndexData[0];
	}
}

std::vector<int> * DeepImage::indexVector(int y, int x) {
	if (0 < x < width() && 0 < y < height()) {
		return &mIndexData[y*width() + x];
	} else {
		return nullptr;
	}
}

std::vector<DeepDataType> DeepImage::renderPixel(int y, int x) const {
	/*
	 * Assumption:
	 * The user wants the data back in the same channel order the deep image was created with.
	 */
	std::map<DeepDataType, int> pixelMap;
	const std::vector<int> indices = mIndexData[y*width() + x];
	for (auto index : indices) {
		DeepDataType z = mChannelData.at(DEPTH)[index];
		pixelMap.insert({z, index});
	}
	std::vector<DeepDataType> values(mChannelData.size() - 1, 0.0);
	// Check if the pixel has any values at all.
	if (pixelMap.begin() == pixelMap.end()) {
		return values;
	}
	auto alphaKeyIter = mChannelData.find(ALPHA);
	if (alphaKeyIter != mChannelData.end()) {
		// If alpha channel does have an alpha channel, composite the pixel together.
		float accumAlpha = 0.0;
		float cutoutAlpha = 1.0f;
		for (auto deepValue = pixelMap.begin(); deepValue != pixelMap.end(); deepValue++) {
			int c = 0;
			float sampleAlpha = mChannelData.at(ALPHA)[deepValue->second];
			if (accumAlpha > cutoutAlpha) {
				break;
			} else if (sampleAlpha < 0.f) {
				// Use + because sampleAlpha is negative to indicate this sample
				// is cutting out from the image.
				cutoutAlpha = cutoutAlpha + sampleAlpha;
			} else {
				// 1st do the alpha channel
				float alpha = std::max(cutoutAlpha - accumAlpha, 0.f)*sampleAlpha;
				// Accumulate the alpha values of the samples
				accumAlpha = accumAlpha + alpha;
				// 2nd do the rest multiplied by the alpha
				for (auto & channelName : mChannelNamesInOrder) {
					if (channelName.compare(ALPHA) == 0) {
						values[c] += alpha;
					} else {
						values[c] += alpha*mChannelData.at(channelName)[deepValue->second];
					}
					c++;
				}
			}
		}
	} else {
		// If deep image doesn't contain an alpha value, just return the top sample value.
		// (should be uncommon/weird).
		auto lastValue = pixelMap.rbegin();
		int c = 0;
		for (auto & channelName : mChannelNamesInOrder) {
			values[c] = mChannelData.at(channelName)[lastValue->second];
			c++;
		}
	}
	return values;
}

void DeepImage::addDeepImage(const DeepImage & other) {
	// Verify the input image has the correct channels.
	for (auto & channelName : mChannelNamesInOrder) {
		if (other.mChannelNamesInOrder.end() ==
				std::find(other.mChannelNamesInOrder.begin(), other.mChannelNamesInOrder.end(), channelName)) {
			std::cerr << "This deep image has a channel " << channelName << " that doesn't exist in the other image" << std::endl;
			return;
		}
	}

	// Verify that the input image has the correct size.
	// TODO


	// Update the index vectors
	int originalNumElems = numElements();
	for (int i = 0; i < mWidth * mHeight; ++i) {
		std::vector<int> & indexVector = mIndexData[i];
		const std::vector<int> & otherIndexVector = other.mIndexData[i];
		indexVector.reserve(indexVector.size() + otherIndexVector.size());
		for (int otherIndex : otherIndexVector) {
			indexVector.push_back(originalNumElems + otherIndex - 1);
		}
	}

	// Append the channel vectors
	for (auto & channelData : mChannelData) {
		std::vector<DeepDataType> & channelVector = channelData.second;
		const std::vector<DeepDataType> & otherChannelVector = other.mChannelData.at(channelData.first);
		// First expand the channel to be able to hold all the new data.
		channelVector.reserve(channelVector.size() + otherChannelVector.size());
		// Then copy the data from other to this channel.
		std::copy(otherChannelVector.begin(), otherChannelVector.end(), std::back_inserter(channelVector));
	}
}

} // End namespace

