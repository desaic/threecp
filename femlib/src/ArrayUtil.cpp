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

int
load_arr(std::vector<Eigen::Vector3d> & a, std::istream & in)
{
  int size = 0;
  in >> size;
  if (size != a.size()){
    std::cout << "Warning: load_arr different array sizes\n";
    return -1;
  }
  a.resize(size);
  for (unsigned int ii = 0; ii<a.size(); ii++){
    for (int jj = 0; jj<a[ii].size(); jj++){
      char inStr[64];
      float val = 0;
      in >> inStr;
      val = atof(inStr);
      a[ii][jj] = val;
    }
  }
  return 0;
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

int gridToLinearIdx(int ix, int iy, int sy)
{
  return ix * sy + iy;
}

void linearToGridIdx(int l, const std::vector<int> & gridSize,
  std::vector<int> & gridIdx)
{
  gridIdx.resize(gridSize.size(), 0);
  for (int i = (int)gridSize.size() - 1; i > 0; i--) {
    gridIdx[i] = l % (gridSize[i]);
    l = l / (gridSize[i]);
  }
  gridIdx[0] = l;
}

bool inbound(int i, int j, int k, const std::vector<int> & s)
{
  return (i >= 0 && i < s[0] && j >= 0 && j < s[1] &&
    k >= 0 && k < s[2]);
}

bool inbound(int i, int j, int si, int sj)
{
  return (i >= 0 && i < si && j >= 0 && j < sj);
}

bool contains(const std::vector<int>& a, int val)
{
  for (size_t i = 0; i < a.size(); i++) {
    if (a[i] == val) {
      return true;
    }
  }
  return false;
}

void addUnique(std::vector<int> & a, int val)
{
  if (!contains(a, val)) {
    a.push_back(val);
  }
}

void remove(std::vector<int> & a, int val)
{
  int idx = -1;
  for (size_t i = 0; i < a.size(); i++) {
    if (a[i] == val) {
      idx = i;
    }
  }
  if (idx < 0) {
    return;
  }
  a[idx] = a[a.size() - 1];
  a.resize(a.size() - 1);
}
