#pragma once

#include "BBox.h"
#include "Vec2.h"
#include "Vec3.h"

class RaycastConf {
 public:
  RaycastConf() : maxTotalHits_(400), voxelSize_(0.032, 0.0635, 0.02) {}

  BBox box_;
  unsigned maxTotalHits_;
  Vec3f voxelSize_;
  /// <summary>
  /// Multiplier onto voxelSize_ for coarse voxel grid size.
  /// The coarse voxel grid is used for acceleration.
  /// </summary>
  float coarseVoxelSizeMult_ = 4;
  /// <summary>
  ///  voxel resolution for coarse acceleration grid.
  /// </summary>
  Vec3f coarseRes_;

  void ComputeCoarseRes() { coarseRes_ = coarseVoxelSizeMult_ * voxelSize_; }
};
