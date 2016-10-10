#include "VoxelIO.hpp"
#include "ArrayUtil.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

void loadBinaryStructure(const std::string & filename,
  std::vector<int> & s,
  std::vector<int> & gridSize)
{
  int dim = 3;
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (!in.good()) {
    std::cout << "Cannot open input " << filename << "\n";
    in.close();
    return;
  }
  gridSize.clear();
  gridSize.resize(dim, 0);
  int nCell = 1;
  for (int i = 0; i < dim; i++) {
    in.read((char*)(&gridSize[i]), sizeof(int));
    nCell *= gridSize[i];
  }
  s.resize(nCell, 0);
  for (std::vector<bool>::size_type i = 0; i < nCell;) {
    unsigned char aggr;
    in.read((char*)&aggr, sizeof(unsigned char));
    for (unsigned char mask = 1; mask > 0 && i < nCell; ++i, mask <<= 1)
      s[i] = (aggr & mask)>0;
  }
  in.close();
}

void loadBinaryStructure(const std::string & filename,
  std::vector<double> & s,
  std::vector<int> & gridSize)
{
  std::vector<int> a;
  loadBinaryStructure(filename, a, gridSize);
  s.resize(a.size(), 0.0);
  for (size_t i = 0; i < a.size(); i++) {
    s[i] = a[i];
  }
}


std::vector<double> mirrorOrthoStructure(const std::vector<double> &s, const std::vector<int> & gridSize)
{
  std::vector<int> newSize = gridSize;
  int nEle = 1;
  for (size_t i = 0; i < newSize.size(); i++) {
    newSize[i] *= 2;
    nEle *= newSize[i];
  }
  std::vector<double> t(nEle);
  for (int i = 0; i < newSize[0]; i++) {
    for (int j = 0; j < newSize[1]; j++) {
      for (int k = 0; k < newSize[2]; k++) {
        int newIdx = gridToLinearIdx(i, j, k, newSize);
        int i0 = i;
        int j0 = j;
        int k0 = k;
        if (i0 >= gridSize[0]) {
          i0 = 2 * gridSize[0] - i0 - 1;
        }
        if (j0 >= gridSize[1]) {
          j0 = 2 * gridSize[1] - j0 - 1;
        }
        if (k0 >= gridSize[2]) {
          k0 = 2 * gridSize[2] - k0 - 1;
        }
        int oldIdx = gridToLinearIdx(i0, j0, k0, gridSize);
        t[newIdx] = s[oldIdx];
      }
    }
  }
  return t;
}

void loadBinDouble(const std::string & filename,
  std::vector<double>&s,
  std::vector<int> & gridSize)
{
  int dim = 3;
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (!in.good()) {
    std::cout << "Cannot open input " << filename << "\n";
    in.close();
    return;
  }
  gridSize.clear();
  gridSize.resize(dim, 0);
  int nCell = 1;
  for (int i = 0; i < dim; i++) {
    in.read((char*)(&gridSize[i]), sizeof(int));
    nCell *= gridSize[i];
  }
  s.resize(nCell, 0);
  std::vector<double> s0(nCell);
  for (int i = 0; i < nCell; i++) {
    float val;
    in.read((char*)&val, sizeof(float));
    s0[i] = val;
  }
  in.close();

  s = mirrorOrthoStructure(s0, gridSize);
  for (size_t i = 0; i < gridSize.size(); i++) {
    gridSize[i] *= 2;
  }
}

void saveBinaryStructure(const std::string & filename,
  const std::vector<int> & s,
  const std::vector<int> & gridSize)
{
  std::ofstream out(filename, std::ios::out | std::ios::binary);
  if (!out.good()) {
    std::cout << "Cannot open output " << filename << "\n";
    out.close();
    return;
  }
  for (size_t i = 0; i < gridSize.size(); i++) {
    out.write((const char *)&gridSize[i], sizeof(int));
  }
  for (size_t i = 0; i < s.size();) {
    unsigned char aggr = 0;
    for (unsigned char mask = 1; mask > 0 && i < s.size(); ++i, mask <<= 1)
      if (s[i])
        aggr |= mask;
    out.write((const char*)&aggr, sizeof(unsigned char));
  }
  out.close();
}

void saveBinaryStructure(const std::string & filename,
  const std::vector<double> & s,
  const std::vector<int> & gridSize)
{
  std::vector<int> a(s.size());
  float thresh = 0.5f;

  for (size_t i = 0; i < a.size(); i++) {
    a[i] = s[i] > thresh;
  }
  saveBinaryStructure(filename, a, gridSize);
}
