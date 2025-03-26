#ifndef CPU_VOXELIZER_H
#define CPU_VOXELIZER_H

#include "TrigMesh.h"
#include "VoxCallback.h"

struct voxconf {
  Vec3f origin;
  Vec3f unit;
  Vec3u gridSize;

};

// ignores voxels with negative indices.
// mesh should be placed above conf.origin.
void cpu_voxelize_mesh(voxconf conf, const TrigMesh* mesh,
                       VoxCallback& cb);

#endif