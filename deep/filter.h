/*
 * filter.h
 *
 *  Created on: Aug 21, 2012
 *      Author: vilhelm
 */

#ifndef FILTER_H_
#define FILTER_H_

namespace deep {

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
	inline float filter(float sx, float sy, int x, int y) const {
		return mOneOver * exp(-(pow(float(sx+0.5)-x, 2.0)+pow(float(sy+0.5)-y, 2.0))/(2.0*mSigma2));
	}
	inline int minX(float x) const { return minPos(x); }
	inline int maxX(float x) const { return maxPos(x); }
	inline int minY(float y) const { return minPos(y); }
	inline int maxY(float y) const { return maxPos(y); }
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
	inline float filter(float sx, float sy, int x, int y) const {
		return 1.0;
	}
	inline int minX(float x) const { return round(x); }
	inline int maxX(float x) const { return round(x); }
	inline int minY(float y) const { return round(y); }
	inline int maxY(float y) const { return round(y); }
};

class LinearFilter : public Filter {
public:
	LinearFilter() : Filter(1) { }
	~LinearFilter() { }
	inline float filter(float sx, float sy, int x, int y) const {
		float distX = std::max(1.0 - fabs(double(x) - sx), 0.0);
		float distY = std::max(1.0 - fabs(double(y) - sy), 0.0);
		return distX * distY;
	}
	inline int minX(float x) const { return floor(x); }
	inline int maxX(float x) const { return ceil(x); }
	inline int minY(float y) const { return floor(y); }
	inline int maxY(float y) const { return ceil(y); }
};

}

#endif /* FILTER_H_ */
