#pragma once
#include <functional>
#include <vector>
#include "Vec3.h"

struct voxconf {
  Vec3f origin;
  Vec3f unit;
  Vec3u gridSize;
};

typedef std::function<void(unsigned x, unsigned y, unsigned z, unsigned long long trigIdx)> VoxCallback;

// Mesh voxelization method
void voxelize_shell(voxconf conf, const std::vector<float>& vertices_in, const std::vector<uint32_t>& triangles_in, VoxCallback cb);