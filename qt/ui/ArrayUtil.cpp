#include "ArrayUtil.hpp"
#include <algorithm>
///@param a bilinear weights in [0 1].

void bilinearWeights(float * a, float * w)
{
  w[0] = (1 - a[0]) * (1 - a[1]);
  w[1] = (a[0]) * (1 - a[1]);
  w[2] = (a[0]) * (a[1]);
  w[3] = (1 - a[0]) * (a[1]);
}

void add_scaled(std::vector<double> & dst, float scale,
  const std::vector<double> & src)
{
  for (unsigned int ii = 0; ii<dst.size(); ii++){
    dst[ii] += scale * src[ii];
  }
}

double infNorm(const std::vector<double> & a){
  double n = 0;
  for (unsigned ii = 0; ii<a.size(); ii++){
    n = std::max(n, (double)std::abs(a[ii]));
  }
  return n;
}

int gridToLinearIdx(int ix, int iy, int iz, const std::vector<int> & gridSize)
{
  return ix * gridSize[1] * gridSize[2] + iy * gridSize[2] + iz;
}

int find(int val , const std::vector<int> & a)
{
  for (int i = 0; i < (int)a.size(); i++){
    if (a[i] == val){
      return i;
    }
  }
  return -1;
}

void transpose2D8(unsigned char * src, unsigned char * dst, int width, int height,
	int src_line_size, int dst_line_size)
{
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			int srcIdx = i * src_line_size + j;
			int dstIdx = j*dst_line_size + i;
			dst[dstIdx] = src[srcIdx];
		}
	}
}