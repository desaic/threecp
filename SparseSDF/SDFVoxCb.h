#ifndef SDF_VOX_CB
#define SDF_VOX_CB

#include <unordered_map>

#include "AdapDF.h"
#include "TrigMesh.h"
#include "VoxCallback.h"
#include "PointTrigDist.h"

// builds intersecting triangle list of
//  coarse voxels around coarse vertices.
// also build mapping from sparse node to data index.
struct TrigListVoxCb : public VoxCallback {
  virtual void operator()(unsigned x, unsigned y, unsigned z,
                          size_t trigIdx) override;
  TrigMesh* m = nullptr;
  AdapDF* df = nullptr;

};
#endif