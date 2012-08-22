/*
 * deep.h
 *
 *  Created on: Aug 1, 2012
 *      Author: vilhelm
 */

#include <math.h>
#include <vector>
#include <array>
#include <map>
#include <string>
#include <iostream>
#include <initializer_list>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <typeinfo>
#include <string.h>

#ifndef DEEP_H_
#define DEEP_H_

namespace deep {

typedef float DeepDataType;
typedef float ImageDataType;

// For saving and loading compatibility, keep track of which version the library a file was saved with.
static const int DEEP_VERSION = 1;

static const std::string ALPHA = "A";
static const std::string DEPTH = "Z";

class Filter {
public:
	Filter(int width) :	mFilterWidth(width) { }
	virtual ~Filter() {}
	virtual int minX(float x) const = 0;
	virtual int maxX(float x) const = 0;
	virtual int minY(float y) const = 0;
	virtual int maxY(float y) const = 0;
	virtual float filter(float sx, float sy, int x, int y) const = 0;
protected:
	int mFilterWidth;
private:
	Filter(const Filter& src);
	Filter& operator=(const Filter& rhs);
};

class GaussianFilter : public Filter {
public:
	GaussianFilter(int width) : Filter(width) {
		float sigma = (mFilterWidth + 1.0)/6.0;
		mSigma2 = sigma * sigma;
		mOneOver = 1.0/(2.0*3.1415926*mSigma2);
	}
	virtual ~GaussianFilter() {}
	virtual float filter(float sx, float sy, int x, int y) const {
		return mOneOver * exp(-(pow(float(sx+0.5)-x, 2.0)+pow(float(sy+0.5)-y, 2.0))/(2.0*mSigma2));
	}
	virtual int minX(float x) const { return minPos(x); }
	virtual int maxX(float x) const { return maxPos(x); }
	virtual int minY(float y) const { return minPos(y); }
	virtual int maxY(float y) const { return maxPos(y); }
private:
	int minPos(float pos) const {
		return int(floor(pos - mFilterWidth/2.0));
	}
	int maxPos(float pos) const {
		return int(ceil(pos + mFilterWidth/2.0));
	}
	float mOneOver;
	float mSigma2;
};

class NNFilter : public Filter {
public:
	NNFilter() : Filter(0) { }
	~NNFilter() { }
	virtual float filter(float sx, float sy, int x, int y) const {
		return 1.0;
	}
	virtual int minX(float x) const { return round(x); }
	virtual int maxX(float x) const { return round(x); }
	virtual int minY(float y) const { return round(y); }
	virtual int maxY(float y) const { return round(y); }
};

class LinearFilter : public Filter {
public:
	LinearFilter() : Filter(1) { }
	~LinearFilter() { }
	virtual float filter(float sx, float sy, int x, int y) const {
		float distX = std::max(1.0 - fabs(double(x) - sx), 0.0);
		float distY = std::max(1.0 - fabs(double(y) - sy), 0.0);
		return distX * distY;
	}
	virtual int minX(float x) const { return floor(x); }
	virtual int maxX(float x) const { return ceil(x); }
	virtual int minY(float y) const { return floor(y); }
	virtual int maxY(float y) const { return ceil(y); }
};


class Image {
public:
	Image(int inWidth, int inHeight, std::vector<std::string> channelNames, std::string pixelFilter = "Nearest");
	~Image();
	ImageDataType * data(int y, int x, int c);
	void addSample(float y, float x, std::initializer_list<ImageDataType> list);
	void addSample(float y, float x, std::vector<ImageDataType> list);
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

Image::Image(int inWidth, int inHeight, std::vector<std::string> inChannelNames, std::string pixelFilter) :
		mWidth(inWidth), mHeight(inHeight), mChannelNames(inChannelNames), mFilter(nullptr) {
	std::cout << "Ctr Image" << std::endl;
	for (auto cn : mChannelNames) {
		std::cout << cn << " ";
	}
	std::cout << std::endl;

	mData = new ImageDataType[width()*height()*channels()];
	std::istringstream iss(pixelFilter);
	std::string type;
	iss >> type;
	if (type.compare("Nearest") == 0) {
		mFilter = new NNFilter();
		std::cout << "Using Nearest" << std::endl;
	} else if (type.compare("Linear") == 0) {
		mFilter = new LinearFilter();
		std::cout << "Using Linear" << std::endl;
	} else { // type == "Gaussian"
		int width = 2;
		iss >> width;
		mFilter = new GaussianFilter(width);
		std::cout << "Using Gaussian" << std::endl;
	}
}

Image::~Image() {
	std::cout << "Dtr Image" << std::endl;
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

class DeepImage {
public:
	DeepImage(int inWidth, int inHeight, std::vector<std::string> channelNames, std::string pixelFilter = "Nearest");
	~DeepImage();
	void addSample(float z, float y, float x, std::initializer_list<DeepDataType> list);
	void addSample(float z, float y, float x, std::vector<DeepDataType> list);
	// void addSampleWithZ(float y, float x, std::vector<DeepDataType> list);
	const std::vector<int> & deepDataIndex(int y, int x) const;
	std::vector<DeepDataType> renderPixel(int y, int x) const;

	inline int channels() const { return mChannelData.size(); }
	std::vector<std::string> channelNames() const {
		std::vector<std::string> names;
		for (auto & channelData : mChannelData) {
			names.push_back(channelData.first);
		}
		return names;
	}
	std::vector<std::string> channelNamesInOrder() const {
		return mChannelNamesInOrder;
	}
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
	const Filter * mFilter;

	friend class DeepImageWriter;
	friend class DeepImageReader;
};


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
		float a = 0.0;
		for (auto deepValue = pixelMap.begin(); deepValue != pixelMap.end(); deepValue++) {
			int c = 0;
			// 1st do the alpha channel
			float alpha = (1.0f - a)*mChannelData.at(ALPHA)[deepValue->second];
			a += alpha;
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

class DeepImageWriter {
public:
	DeepImageWriter(std::string filename, const DeepImage & image) : mFilename(filename), mDeepImage(image), mFileHandle(nullptr) { }
	virtual ~DeepImageWriter() { }
	bool open();
	bool close();
	bool write();
private:
	DeepImageWriter(const DeepImageWriter & src);
	DeepImageWriter & operator=(const DeepImageWriter & rhs);
	std::string mFilename;
	std::ofstream * mFileHandle;
	const DeepImage & mDeepImage;
};

bool DeepImageWriter::open() {
	close();  // Close any already-opened file
	mFileHandle = new std::ofstream(mFilename.c_str(), std::ios_base::out | std::ios_base::binary);
	if (!mFileHandle) {
		std::cerr << "Could not open file " << mFilename << std::endl;
		return false;
	}

	if (!mFileHandle->good()) {
		std::cerr << "Could not open file " << mFilename << std::endl;
	}

	mFileHandle->write(reinterpret_cast<const char *>(&DEEP_VERSION), sizeof(int));
//	mFileHandle->write(typeid(DeepDataType).name(), strlen(typeid(DeepDataType).nam/e()));
	mFileHandle->write(reinterpret_cast<const char *>(&mDeepImage.mWidth), sizeof(int));
	mFileHandle->write(reinterpret_cast<const char *>(&mDeepImage.mHeight), sizeof(int));
	int numElems = mDeepImage.numElements();
	mFileHandle->write(reinterpret_cast<char *>(&numElems), sizeof(int));
	for (auto & channelData : mDeepImage.mChannelData) {
		mFileHandle->write(channelData.first.c_str(), sizeof(char)*(channelData.first.size() + 1));
	}
	char newline = '\n';
	mFileHandle->write(&newline, sizeof(char));
	for (auto & channelName : mDeepImage.mChannelNamesInOrder) {
		mFileHandle->write(channelName.c_str(), sizeof(char)*(channelName.size() + 1));
	}
	mFileHandle->write(&newline, sizeof(char));
}

bool DeepImageWriter::write() {
	int breakInt = -1;
	for (int i = 0; i < mDeepImage.width() * mDeepImage.height(); ++i) {
		for (int index : mDeepImage.mIndexData[i]) {
			mFileHandle->write(reinterpret_cast<char *>(&index), sizeof(int));
		}
		mFileHandle->write(reinterpret_cast<char *>(&breakInt), sizeof(int));
	}

	for (auto & channelData : mDeepImage.mChannelData) {
		int channelSize = channelData.second.size();
		std::cout << "Writing channel " << channelData.first << " data size: " << channelSize << std::endl;
		mFileHandle->write(reinterpret_cast<char *>(&channelSize), sizeof(int));
		for (auto & value : channelData.second) {
			mFileHandle->write(reinterpret_cast<const char *>(&value), sizeof(DeepDataType));
		}
	}
}

bool DeepImageWriter::close() {
	if (mFileHandle) {
		mFileHandle->close();
		delete mFileHandle;
		mFileHandle = nullptr;
	}
	return true;
}

class DeepImageReader {
public:
	DeepImageReader(std::string filename) : mFilename(filename) { }
	virtual ~DeepImageReader() { }
	DeepImage * read();
private:
	DeepImageReader(const DeepImageReader & src);
	DeepImageReader & operator=(const DeepImageReader & rhs);
	std::string mFilename;
};

std::string readNullTermString(std::ifstream & fileHandle) {
	std::string value;
	while (true) {
		char c;
		fileHandle.read(&c, sizeof(char));
		if (c != '\0') { value.push_back(c); }
		else { return value; }
	}
}

DeepImage * DeepImageReader::read() {
	std::ifstream mFileHandle(mFilename.c_str(), std::ios_base::out | std::ios_base::binary);
	if (!mFileHandle) {
		std::cerr << "Could not open file " << mFilename << std::endl;
		return nullptr;
	}

	if (!mFileHandle.good()) {
		std::cerr << "Could not open file " << mFilename << std::endl;
		return nullptr;
	}

	// Verify file version
	int version;
	mFileHandle.read(reinterpret_cast<char *>(&version), sizeof(int));
	if (version > DEEP_VERSION) {
		std::cerr << "Trying to load a file that was saved with a newer version of this library" << std::endl;
		return nullptr;
	}

//	// Check file type
//	std::string deepDataType;
//	mFileHandle->read(reinterpret_cast<char *>(&deepDataType), strlen(typeid(DeepDataType).name()));

	// Read some basic info
	int width, height;
	mFileHandle.read(reinterpret_cast<char *>(&width), sizeof(int));
	mFileHandle.read(reinterpret_cast<char *>(&height), sizeof(int));
	int numElems;
	mFileHandle.read(reinterpret_cast<char *>(&numElems), sizeof(int));

	std::cout << "Width: " << width << " height " << height << " num elems: " << numElems << std::endl;

	// Read channel info
	std::vector<std::string> channelNames;
	bool channelsRead = false;
	while (!channelsRead) {
		channelNames.push_back(readNullTermString(mFileHandle));
		if (mFileHandle.peek() == static_cast<int>('\n')) {
			channelsRead = true;
			// Read the newline but don't use it for anything.
			char c; mFileHandle.read(&c, sizeof(char));
		}
	}

	std::vector<std::string> channelNamesInOrder;
	channelsRead = false;
	while (!channelsRead) {
		channelNamesInOrder.push_back(readNullTermString(mFileHandle));
		if (mFileHandle.peek() == static_cast<int>('\n')) {
			channelsRead = true;
			// Read the newline but don't use it for anything.
			char c; mFileHandle.read(&c, sizeof(char));
		}
	}

	std::cout << "Channel names are:" << std::endl;
	for (auto & str : channelNames) {
		std::cout << str << " ";
	}

	std::cout << "Channel names in order are:" << std::endl;
	for (auto & str : channelNamesInOrder) {
		std::cout << str << " ";
	}

	DeepImage * image = new DeepImage(width, height, channelNamesInOrder);

	for (int i = 0; i < width * height; ++i) {
		while (true) {
			int idx;
			mFileHandle.read(reinterpret_cast<char *>(&idx), sizeof(int));
			if (idx != -1) {
				image->mIndexData[i].push_back(idx);
			} else {
				break;
			}
		}
	}

	for (auto & channelData : image->mChannelData) {
		int channelSize;
		mFileHandle.read(reinterpret_cast<char *>(&channelSize), sizeof(int));
		// Request a change in size of the vector, in anticipation for the channel data.
		channelData.second.reserve(channelSize);
		for (int i = 0; i < channelSize; ++i) {
			DeepDataType value;
			mFileHandle.read(reinterpret_cast<char *>(&value), sizeof(DeepDataType));
			channelData.second.push_back(value);
		}
	}

	// Close the file.
	if (mFileHandle) {
		mFileHandle.close();
	}
	return image;
}

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

}

#endif /* DEEP_H_ */
