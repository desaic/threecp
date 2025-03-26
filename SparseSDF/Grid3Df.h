#pragma once

#include "Array3D.h"
namespace slicer {
///@brief Array3Df with origin, voxel size.
/// also has saving and loading
class Grid3Df {
 public:
  void Allocate(const Vec3u &size, float initVal) {
    _arr.Allocate(size, initVal);
  }
  float Trilinear(const Vec3f& x) const;
  Vec3f origin = {};
  Vec3f voxelSize = {1.0f, 1.0f, 1.0f};
  Vec3u GetSize() const { return _arr.GetSize(); }
  float& operator()(unsigned int x, unsigned int y, unsigned int z) { return _arr(x, y, z); }

  float operator()(unsigned int x, unsigned int y, unsigned int z) const { return _arr(x, y, z); }

  const Array3Df& Array() const { return _arr; }
  Array3Df& Array() { return _arr; }
 private:
  Array3Df _arr;
};

}  // namespace slicer
