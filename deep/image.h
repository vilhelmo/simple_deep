/*
 * image.h
 *
 *  Created on: Aug 21, 2012
 *      Author: vilhelm
 */

#ifndef IMAGE_H_
#define IMAGE_H_

#include "deep.h"

namespace deep {

// Forward declarations
class Filter;

class Image {
public:
	Image(int inWidth, int inHeight, std::vector<std::string> channelNames, std::string pixelFilter = "Nearest");
	~Image();
	ImageDataType * data(int y, int x, int c);
	ImageDataType data(int y, int x, int c) const;
	void addSample(float y, float x, std::initializer_list<ImageDataType> list);
	void addSample(float y, float x, std::vector<ImageDataType> list);
	inline const std::vector<std::string> & channelNames() const { return mChannelNames; }
	inline int channels() const { return mChannelNames.size(); }
	inline int width() const { return mWidth; }
	inline int height() const { return mHeight; }
private:
	Image(const Image& src);
	Image& operator=(const Image& rhs);
	const int mWidth, mHeight;
	const std::vector<std::string> mChannelNames;
	ImageDataType * mData;
	const Filter * mFilter;
};


} // End namespace

#endif /* IMAGE_H_ */
