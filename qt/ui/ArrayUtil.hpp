#ifndef ARRAYUTIL_HPP
#define ARRAYUTIL_HPP

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

//min in array util. to avoid naming conflict
#define MIN_AU(a,b)  ((a) > (b) ? (b) : (a))
template <typename T>
inline T clamp(T i, T lb, T ub)
{
	if (i < lb) { i = lb; }
	if (ub < i) { i = ub; }
	return i;
}

///@brief for Vector3f/d
template <typename T>
int
save_arr(const std::vector<T> & a, std::ostream & out)
{
  out<<a.size()<<"\n";
  for(unsigned int ii = 0; ii<a.size(); ii++){
    for(int jj = 0; jj<a[ii].size(); jj++){
      if(jj>0){
        out<<" ";
      }
      out<<a[ii][jj];
    }
    out<<"\n";
  }
  return 0;
}

///@param a bilinear weights in [0 1].
void bilinearWeights(float * a, float * w);

template <typename T>
void printVector(const std::vector<T> & v, std::ostream & out)
{
  for(unsigned int ii =0 ; ii<v.size(); ii++){
    out<<v[ii]<<"\n";
  }
}

template <typename T>
void BBox(const std::vector<T>& v, T & mn, T & mx)
{
  mn = v[0];
  mx = v[0];
  int DIM = (int)v[0].size();
  for(unsigned int ii = 1 ;ii<v.size();ii++){
    for(int dim = 0; dim < DIM; dim++){
      if(v[ii][dim]<mn[dim]){
        mn[dim] = v[ii][dim];
      }
      if(v[ii][dim]>mx[dim]){
        mx[dim] = v[ii][dim];
      }
    }
  }
}

template<typename T>
void add(std::vector<T> & dst, const std::vector<T> & src)
{
  for(unsigned int ii = 0;ii<dst.size();ii++){
    dst[ii] += src[ii];
  }
}

void add_scaled(std::vector<double> & dst, float scale,
  const std::vector<double> & src);

double infNorm(const std::vector<double> & a);

template<typename T>
std::vector<T> mul(float f, const std::vector<T> & src)
{
  std::vector<T> prod(src.size());
  for(unsigned int ii = 0;ii<src.size();ii++){
    prod[ii] = f*src[ii];
  }
  return prod;
}

template<typename T>
void addmul(std::vector<T> & dst, float f, const std::vector<T> & src)
{
  for(unsigned int ii = 0;ii<src.size();ii++){
    dst[ii] += f*src[ii];
  }
}

template<typename T>
void setVal(std::vector<T> & a, const std::vector<int> & c, const T & val)
{
  for(unsigned int ii =0 ; ii<a.size(); ii++){
    if(c[ii]){
      a[ii] = val;
    }
  }
}

int gridToLinearIdx(int ix, int iy, int iz, const std::vector<int> & gridSize);

inline uint64_t gridToLinearIdx(int ix, int iy, int iz, const int* gridSize)
{
	return ix * (uint64_t)gridSize[1] * gridSize[2] + iy * (uint64_t)gridSize[2] + iz;
}

inline uint64_t gridToLinearIdx(int ix, int iy, int iz, size_t sx, size_t sy, size_t sz)
{
	return ix * (uint64_t)sy*sz + iy * (uint64_t)sz + iz;
}

int find(int val, const std::vector<int> & a);

//transpose a 2d array of 8-bit ints.
void transpose2D8(unsigned char * src, unsigned char * dst, int width, int height,
	int src_line_size, int dst_line_size);

template<typename T>
T trilinearInterp(float x, float y, float z,
	const std::vector<T> & a, size_t sx, size_t sy, size_t sz) {
	uint32_t i = MIN_AU((uint32_t)x, sx - 2);
	uint32_t j = MIN_AU((uint32_t)y, sy - 2);
	uint32_t k = MIN_AU((uint32_t)z, sz - 2);;
	float w[3];
	w[0] = x-i;
	w[1] = y-j;
	w[2] = z-k;
	T val = (1 - w[0])*(1 - w[1])*(1 - w[2])*a[gridToLinearIdx(i, j, k, sx, sy, sz)]
		+ (1 - w[0])*(1 - w[1])*(w[2])*a[gridToLinearIdx(i, j, k+1, sx, sy, sz)]
		+ (1 - w[0])*(w[1])*(1 - w[2])*a[gridToLinearIdx(i, j + 1, k, sx, sy, sz)]
		+ (1 - w[0])*(w[1])*(w[2])*a[gridToLinearIdx(i, j + 1, k + 1, sx, sy, sz)]
		+ (w[0])*(1 - w[1])*(1 - w[2])*a[gridToLinearIdx(i + 1, j, k, sx, sy, sz)]
		+ (w[0])*(1 - w[1])*(w[2])*a[gridToLinearIdx(i + 1, j, k + 1, sx, sy, sz)]
		+ (w[0])*(w[1])*(1 - w[2])*a[gridToLinearIdx(i + 1, j + 1, k, sx, sy, sz)]
		+ (w[0])*(w[1])*(w[2])*a[gridToLinearIdx(i + 1, j + 1, k + 1, sx, sy, sz)];
	return val;
}

template <typename T>
std::vector<T>
resize3D(std::vector<T>a, size_t sx, size_t sy, size_t sz,
	size_t sx1, size_t sy1, size_t sz1) {
	std::vector<T> x(sx1*sy1*sz1);
	for (size_t i = 0; i < sx1; i++) {
		for (size_t j = 0; j < sy1; j++) {
			for (size_t k = 0; k < sz1; k++) {
				x[gridToLinearIdx(i, j, k, sx1, sy1, sz1)] = trilinearInterp( (sx-1) * i / (float)(sx1 - 1),
					(sy-1) * j / (float)(sy1 - 1), (sz-1) * k / (float)(sz1 - 1), a, sx, sy, sz);
			}
		}
	}
	return x;
}

#endif // ARRAYUTIL_HPP
