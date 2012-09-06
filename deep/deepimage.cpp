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
#include <exception>

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

//	std::cout << "Deep Image Constructor" << std::endl;

	bool hasZ = false;
	mIndexData = new std::vector<int>[width()*height()];
	for (auto channelName : mChannelNamesInOrder) {
//		std::cout << "\tCreating channel " << channelName << std::endl;
		mChannelData.insert({channelName, std::vector<DeepDataType>()});
		if (channelName.compare(DEPTH) == 0) {
			hasZ = true;
		} else if (channelName.compare(DEPTH_BACK) == 0) {
			this->mHasZBack = true;
		} else {
			mChannelNamesNoZs.push_back(channelName);
		}
	}
	if (!hasZ) {
		std::cerr << "Didn't specify a Z channel for the Deep image. A flat image should use the Image class." << std::endl;
		throw std::exception();
//		mChannelData.insert({DEPTH, std::vector<DeepDataType>()});
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
	int index = mChannelData[DEPTH].size() - 1;
	indexVector(iy, ix)->push_back(index);
}

void DeepImage::addSample(float y, float x, std::vector<DeepDataType> list) {
	if (list.size() != mChannelData.size()) { return; }
	int iy = std::max(std::min(int(y * height()), height() - 1), 0);
	int ix = std::max(std::min(int(x * width()), width() - 1), 0);

	auto channelNameIter = mChannelNamesInOrder.begin();
	for (auto inputIter = list.begin(); inputIter != list.end(); ++inputIter) {
		mChannelData[*channelNameIter].push_back(*inputIter);
		channelNameIter++;
	}
	int index = mChannelData[DEPTH].size() - 1;
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

DeepDataType evalFunc(const std::array<DeepDataType, 3> & f, DeepDataType x) {
	if (x <= f[0]) {
		return 1.0;
	} else if (x >= f[1]) {
		return f[2];
	} else {
		// lin interp
		return 1.0 - (x - f[0])*(1.0 - f[2])/(f[1] - f[0]);
	}
}

inline DeepDataType limitValue(DeepDataType value) {
	return std::min(std::max(value, DeepDataType(0.0)), DeepDataType(1.0));
}

std::vector<DeepDataType> DeepImage::renderPixelLinear(int y, int x) const {
	/*
	 * Assumption: Image has at least the following channels:
	 * DEPTH
	 * DEPTH_BACK
	 * ALPHA
	 */

	// TODO: Verify image has the correct channels (Z, ZBack, A)

	// Discontinuous transmittance function, 1=transparent, 0=opaque
	// key is the z-value
	// the array is three values:
	// 1. is the "top" or high transmittance value
	// 2. is the "botton" or low transmittance value
	// 3. is a counter for how many volumes that share this sample.
	std::map<DeepDataType, std::array<DeepDataType, 3>> transFunc;

	// Each flat surface or volume is stored as a simple 3 value
	// function is this vector
	// The first value is z, second is zBack and third is 1.0-alpha.
	std::vector<std::array<DeepDataType, 3>> sampleFuncs;

	// Initialize the transFunc by adding samples.
	const std::vector<int> indices = mIndexData[y*width() + x];
	for (auto index : indices) {
		DeepDataType z = mChannelData.at(DEPTH)[index];
		DeepDataType zBack = mChannelData.at(DEPTH_BACK)[index];
		DeepDataType alpha = mChannelData.at(ALPHA)[index];

		// Add this function to the sampleFuncs list.
		sampleFuncs.push_back({{z, zBack, (1.f-std::fabs(alpha))}});

		// Add or fetch the current sample from the main transmittance func.
		std::array<DeepDataType, 3> & t = transFunc[z];
		t[0] = 1.0; // Initialize first value (top)
		if (zBack > z) {
			if (std::fabs(t[1]) < EPSILON) {
				// If the bottom value is uninialized, set it to 1
				// to indicate this is a continous point.
				t[1] = 1.0;
			}
			// Initialize the end point.
			std::array<DeepDataType, 3> & t2 = transFunc[zBack];
			t2[0] = 1.0;
			if (std::fabs(t2[1]) < EPSILON) {
				t2[1] = 1.0; // Continous point.
			}
		} else {
			// Initialize the bottom value to -1.0 to indicate this is a discontinous point.
			t[1] = -1.0;
		}
	}

	// Initialize the final pixel
	std::vector<DeepDataType> finalColorValues(mChannelNamesNoZs.size(), 0.0);

	// Check if the pixel has any values at all.
	if (transFunc.begin() == transFunc.end()) {
		return finalColorValues;
	}

	// Compute the transmittance function by multiplying in every sample function.
	for (auto & func : sampleFuncs) {
		// Get the starting point.
		auto transIter = transFunc.find(func[0]);

		if (transIter != transFunc.end()) { // Just for safety.
			// Get the sample
			std::array<DeepDataType, 3> & transmittanceSample = transIter->second;

			if (std::fabs(func[1] - func[0]) < EPSILON) {
				// If the sample is a discontinous point, i.e. flat surface.
				if (transmittanceSample[1] >= 0.f) {
					// This points low is initialized, so just multiply the
					// current function with the value.
					transmittanceSample[1] = transmittanceSample[1] * func[2];
				} else {
					// This point is uninitialized (set to < 0), so use the
					// high value multiplied by the current functions transmittance value.
					transmittanceSample[1] = transmittanceSample[0] * func[2];
				}
			} else {
				// This is a volume point, so increase the volume counter by one.
				transmittanceSample[2] += 1.0;
			}

			transIter++;
			while (transIter != transFunc.end()) {
				// For the rest of the points, multiply with the function value
				DeepDataType transDepth = transIter->first;
				// Evaluate the current function value at the current depth.
				DeepDataType transparency = evalFunc(func, transDepth);
				// Multiply both the high and low value with the current value.
				std::array<DeepDataType, 3> & transmittanceSample = transIter->second;
				transmittanceSample[0] *= transparency;
				transmittanceSample[1] *= transparency;
				if (transDepth < func[1]) {
					// If this sample is within the volume, increase the counter.
					transmittanceSample[2] += 1.0;
				}
				transIter++;
			}
		}
	}

//	// Debug the transmittance func
//	for (auto & trans : transFunc) {
//		std::cout << "z:" << trans.first << " hi:" << trans.second[0] <<
//				" lo:" << trans.second[1] << " num:" << trans.second[2] << std::endl;
//	}

	DeepDataType minTrans = 1.0;
	// Do the final compositing using the transmittance function.
	// TODO: Comment this part.
	for (auto index : indices) {
		DeepDataType z = mChannelData.at(DEPTH)[index];
		DeepDataType zBack = mChannelData.at(DEPTH_BACK)[index];
		DeepDataType alpha = mChannelData.at(ALPHA)[index];
		if (alpha >= 0.0) {
			auto transIter = transFunc.find(z);
			if (zBack > z) {
				DeepDataType lastTrans = transIter->second[1];
				DeepDataType transparency = 0.0;
				DeepDataType volumeCounter = transIter->second[2];
				auto transBackIter = transFunc.find(zBack);
				do {
					transIter++;
					transparency += (lastTrans - transIter->second[0])/volumeCounter;
					volumeCounter = transIter->second[2];
					lastTrans = transIter->second[1];
				} while (transIter != transBackIter);
				minTrans = std::min(minTrans, transparency);
				int c = 0;
				for (auto & channelName : mChannelNamesNoZs) {
					if (channelName.compare(ALPHA) != 0) {
						finalColorValues[c] += transparency*mChannelData.at(channelName)[index];
					} else {
						finalColorValues[c] += transparency;
					}
					c++;
				}
			} else {
				DeepDataType transDiff = transIter->second[0] - transIter->second[1];
				minTrans = std::min(minTrans, transIter->second[1]);
				int c = 0;
				for (auto & channelName : mChannelNamesNoZs) {
					if (channelName.compare(ALPHA) != 0) {
						finalColorValues[c] += transDiff*mChannelData.at(channelName)[index];
					} else {
						finalColorValues[c] += transDiff;
					}
					c++;
				}
			}
		}
	}

	// Unpremult
	auto alphaIter = std::find(mChannelNamesNoZs.begin(), mChannelNamesNoZs.end(), ALPHA);
	DeepDataType lastAlpha = finalColorValues[std::distance(mChannelNamesNoZs.begin(), alphaIter)];
	int c = 0;
	for (auto & channelName : mChannelNamesNoZs) {
		if (channelName.compare(ALPHA) != 0) {
			if (lastAlpha > 0.0) {
				finalColorValues[c] /= lastAlpha;
			} else {
				finalColorValues[c] = 0.0;
			}
		}
		c++;
	}

	return finalColorValues;
}

std::vector<DeepDataType> DeepImage::renderPixel(int y, int x) const {
	/*
	 * Assumption:
	 * The user wants the data back in the same channel order the deep image was created with.
	 * The channels Z and ZBack are reserved and will not be composited together,
	 * they're used for the compositing itself.
	 */
	std::map<DeepDataType, int> pixelMap;
	const std::vector<int> indices = mIndexData[y*width() + x];
	for (auto index : indices) {
		DeepDataType z = mChannelData.at(DEPTH)[index];
		pixelMap.insert({z, index});
	}
	std::vector<DeepDataType> values(mChannelNamesNoZs.size(), 0.0);
	// Check if the pixel has any values at all.
	if (pixelMap.begin() == pixelMap.end()) {
		return values;
	}
	auto alphaKeyIter = mChannelData.find(ALPHA);
	if (alphaKeyIter != mChannelData.end()) {
		// If alpha channel does have an alpha channel, composite the pixel together.
		float accumAlpha = 0.0;
		float cutoutAlpha = 1.0;
		for (auto deepValue = pixelMap.begin(); deepValue != pixelMap.end(); deepValue++) {
			int c = 0;
			float sampleAlpha = mChannelData.at(ALPHA)[deepValue->second];
			if (accumAlpha > cutoutAlpha) {
				break;
			} else if (sampleAlpha < 0.0) {
				// Use + because sampleAlpha is negative to indicate this sample
				// is cutting out from the image.
				cutoutAlpha = cutoutAlpha + sampleAlpha;
			} else {
				// 1st do the alpha channel
				float alpha = std::max(cutoutAlpha - accumAlpha, 0.f)*sampleAlpha;
				// Accumulate the alpha values of the samples
				accumAlpha = accumAlpha + alpha;
				// 2nd do the rest multiplied by the alpha
				for (auto & channelName : mChannelNamesNoZs) {
					if (channelName.compare(ALPHA) == 0) {
						values[c] += alpha;
					} else {
						values[c] += alpha*mChannelData.at(channelName)[deepValue->second];
					}
					c++;
				}
			}
		}

		// Unpremult
		int c = 0;
		for (auto & channelName : mChannelNamesNoZs) {
			if (channelName.compare(ALPHA) != 0) {
				values[c] /= accumAlpha;
			}
			c++;
		}

	} else {
		// If deep image doesn't contain an alpha value, just return the top sample value.
		// (should be uncommon/weird).
		auto lastValue = pixelMap.rbegin();
		int c = 0;
		for (auto & channelName : mChannelNamesNoZs) {
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
	if (mWidth != other.mWidth || mHeight != other.mHeight) {
		std::cerr << "The image sizes doesn't match up." << std::endl;
		return;
	}

	// Update the index vectors
	int originalNumElems = numElements();
	for (int i = 0; i < mWidth * mHeight; ++i) {
		std::vector<int> & indexVector = mIndexData[i];
		const std::vector<int> & otherIndexVector = other.mIndexData[i];
		indexVector.reserve(indexVector.size() + otherIndexVector.size());
		for (int otherIndex : otherIndexVector) {
			indexVector.push_back(originalNumElems + otherIndex); // - 1);
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

void DeepImage::subtractDeepImage(const DeepImage & other) {
	int originalNumElems = numElements();
	addDeepImage(other);
	// Invert the alpha values of all the added samples
	// to indicate that they should be subtracted when rendering each pixel.
	std::vector<DeepDataType> & alphaValues = mChannelData.at(ALPHA);
	for (int i = originalNumElems; i < numElements(); ++i) {
		alphaValues[i] = -1.f * alphaValues[i];
	}
}


} // End namespace

