#include "Array3D.h"
#include "cpu_voxelizer.h"

struct SimpleVoxCb : public VoxCallback {
  virtual void operator()(unsigned x, unsigned y, unsigned z,
                          size_t trigIdx) {
    (*grid)(x, y, z) = matId;
    //update distance values in its 
  }
  Array3D8u* grid = nullptr;
  uint8_t matId=1;
};
