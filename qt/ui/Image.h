#pragma once
#include <array>
#include <vector>
#include <stdint.h>

///\brief x or width direction is depth.
///row-major order.
template<typename T>
struct Image {
	Image() {}
	Image(const Image<T> & img) :data(img.data) { size = img.size; }
	Image(const std::vector<T> img, const std::array<uint32_t, 2> & imgSize) :
		data(img), size(imgSize) {}
	std::vector<T> data;
	///\brief width and height.
	std::array<size_t, 2> size;
	void allocate() { data.resize(size[0] * size[1]); }
	T operator[](size_t idx) const { return data[idx]; }
	T & operator[](size_t idx) { return data[idx]; }

	T operator()(size_t x, size_t y) const { return data[x + y * size[0]]; }
	T & operator()(size_t x, size_t y) { return data[x + y * size[0]]; }
};


typedef Image<uint8_t> ImageU8;
typedef Image<uint16_t> ImageU16;
typedef Image<float> ImageF32;

///\param margin margin of image to exclude from plane fitting.
void fitZPlane(const ImageF32 & image, double & a, double &b, double & zcenter, int margin = 0);
void transformImage(const ImageF32 & input, ImageF32 & output, double a, double b);
void rotateImage(const ImageF32 & input, ImageF32 & output, double angle);