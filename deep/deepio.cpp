/*
 * deepio.cpp
 *
 *  Created on: Aug 21, 2012
 *      Author: vilhelm
 */

#include <string>
#include <vector>
#include <iostream>
#include <zlib.h>
#include "deepio.h"
#include "deepimage.h"

namespace deep {


// Helper function to read a null terminated c-str from an ifstream.
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

//	std::cout << "Reading deep file: " << mFilename << std::endl;

//	// Check file type
//	std::string deepDataType;
//	mFileHandle->read(reinterpret_cast<char *>(&deepDataType), strlen(typeid(DeepDataType).name()));

	// Read some basic info
	int width, height;
	mFileHandle.read(reinterpret_cast<char *>(&width), sizeof(int));
	mFileHandle.read(reinterpret_cast<char *>(&height), sizeof(int));
	int numElems;
	mFileHandle.read(reinterpret_cast<char *>(&numElems), sizeof(int));

//	std::cout << "\tWidth: " << width << " height " << height << " num elems: " << numElems << std::endl;

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

//	std::cout << "\tChannel names are: ";
//	for (auto & str : channelNames) {
//		std::cout << str << " ";
//	}
//
//	std::cout << std::endl << "\tChannel names in order are: ";
//	for (auto & str : channelNamesInOrder) {
//		std::cout << str << " ";
//	}
//	std::cout << std::endl;

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
//		std::cout << "Writing channel " << channelData.first << " data size: " << channelSize << std::endl;
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


} // End namespace
