#include "Grid3Df.h"

#include "Array3DUtils.h"
namespace slicer {
float Grid3Df::Trilinear(const Vec3f& x) const {
  return TrilinearInterpWithDefault(_arr, x - origin, voxelSize, 0.0f);
}
}  // namespace slicer
