/*
 * deepimage.h
 *
 *  Created on: Aug 21, 2012
 *      Author: vilhelm
 */

#ifndef DEEPIMAGE_H_
#define DEEPIMAGE_H_

#include "deep.h"

namespace deep {

// Forward declares
class Filter;

class DeepImage {
public:
	DeepImage(int inWidth, int inHeight, std::vector<std::string> channelNames, std::string pixelFilter = "Nearest");
	~DeepImage();

	// Add another deep image, will require that all channels in this
	// deep image exists in the other image. Extra channels will be discarded.
	void addDeepImage(const DeepImage & other);
	void addSample(float z, float y, float x, std::initializer_list<DeepDataType> list);
	void addSample(float z, float y, float x, std::vector<DeepDataType> list);
	// void addSampleWithZ(float y, float x, std::vector<DeepDataType> list);
	const std::vector<int> & deepDataIndex(int y, int x) const;
	std::vector<DeepDataType> renderPixel(int y, int x) const;

	inline int channels() const { return mChannelData.size(); }
	inline int channelsInOrder() const { return mChannelNamesInOrder.size(); }
	std::vector<std::string> channelNames() const {
		std::vector<std::string> names;
		for (auto & channelData : mChannelData) {
			names.push_back(channelData.first);
		}
		return names;
	}
	inline std::vector<std::string> channelNamesInOrder() const { return mChannelNamesInOrder; }
	inline int width() const { return mWidth; }
	inline int height() const { return mHeight; }
	int numElements() const { return mChannelData.at(DEPTH).size(); }
	int maxElementsInPixel() const {
		int max = 0;
		for (int i = 0; i < width()*height(); ++i) {
			max = std::max(max, int(mIndexData[i].size()));
		}
		return max;
	}
private:
	DeepImage(const DeepImage& src);
	DeepImage& operator=(const DeepImage& rhs);

	std::vector<int> * indexVector(int y, int x);

	const int mWidth, mHeight;
	const std::vector<std::string> mChannelNamesInOrder;
	std::map<std::string, std::vector<DeepDataType>> mChannelData;
	std::vector<int> * mIndexData;
	const Filter * mFilter; // TODO: NOT USED at the moment.

	friend class DeepImageWriter;
	friend class DeepImageReader;
};


} // End namespace


#endif /* DEEPIMAGE_H_ */
