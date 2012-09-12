/*
 * deepio.h
 *
 *  Created on: Aug 21, 2012
 *      Author: vilhelm
 */

#ifndef DEEPIO_H_
#define DEEPIO_H_

#include <fstream>
#include "deepimage.h"

namespace deep {

class DeepImage;

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


class DeepImageWriter {
public:
	DeepImageWriter(std::string filename, const DeepImage & image) : mFilename(filename), mDeepImage(image), mFileHandle(nullptr) { }
	virtual ~DeepImageWriter() { }
	bool open();
	void close();
	void write();
private:
	DeepImageWriter(const DeepImageWriter & src);
	DeepImageWriter & operator=(const DeepImageWriter & rhs);
	std::string mFilename;
	std::ofstream * mFileHandle;
	const DeepImage & mDeepImage;
};


} // End namespace


#endif /* DEEPIO_H_ */
