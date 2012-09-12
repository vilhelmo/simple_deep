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
#include <limits>

#ifndef DEEP_H_
#define DEEP_H_

namespace deep {

//typedef float DeepDataType;
//typedef float ImageDataType;
typedef double DeepDataType;
typedef double ImageDataType;

// For saving and loading compatibility, keep track of which version the library a file was saved with.
static const int DEEP_VERSION = 1;

static const std::string ALPHA = "A";
static const std::string DEPTH = "Z";
static const std::string DEPTH_BACK = "ZBack";

static const DeepDataType EPSILON = std::numeric_limits<DeepDataType>::min(); //0.0000001;

// Forward declares.
class Image;
class DeepImage;

// Helper functions:
void printDeepImageStats(const DeepImage & image);
void printFlatImageStats(const Image & image);
Image * renderDeepImage(const DeepImage & deepImage);

} // End namespace

#endif /* DEEP_H_ */
