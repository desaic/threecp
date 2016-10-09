#include "VoxelIO.hpp"
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
