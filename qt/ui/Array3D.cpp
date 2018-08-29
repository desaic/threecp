#include "Array3D.h"
#include <iostream>
#include <fstream>
#include <string>
Array3D::Array3D():pixelBytes(1)
{
}

Array3D::~Array3D()
{}

void Array3D::loadFile(const std::string & filename)
{
  std::ifstream in(filename);
  if (!in.good()) {
    std::cout << "can not open " << filename << "\n";
    return;
  }
  in.read((char*)(&size), sizeof(size));
  in.read((char*)(&pixelBytes), sizeof(pixelBytes));
  size_t nPixels = size[0] * size[1] * size[2];
  size_t nBytes = nPixels * pixelBytes;
  data.resize(nBytes);
  in.read((char*)data.data(), nBytes);
  in.close();
}

void Array3D::saveFile(const std::string & filename)
{
  std::ofstream out(filename);
  if (!out.good()) {
    std::cout << "can not open " << filename << "\n";
    return;
  }
  out.write((char*)(&size), sizeof(size));
  out.write((char*)(&pixelBytes), sizeof(pixelBytes));
  out.write((char*)data.data(), data.size());
  out.close();
}

uint8_t & Array3D::operator()(int x, int y, int z)
{
  size_t linearIdx = pixelBytes * ( (x * size[1] + y) * size[2] + z);
  return data[linearIdx];
}
