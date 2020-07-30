#pragma once
#include <vector>

class Array3D {
public:
  Array3D();
  ~Array3D();

  void loadFile(const std::string & filename);
  void saveFile(const std::string & filename);
  uint8_t & operator()(int x, int y, int z);
private:
  size_t size[3] = { 0 };
  //how many bytes does a pixel use.
  int pixelBytes;
  std::vector<uint8_t> data;
};