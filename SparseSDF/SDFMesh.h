#pragma once

#include <memory>

#include "BBox.h"
#include "Vec3.h"

class TrigMesh;

struct SDFInfo {
  /// multiples of h
  float band;

  /// h. only supports uniform voxel size.
  float voxelSize;

  //  computed quantities

  BBox box;

  SDFInfo() : band(3), voxelSize(0.4f){}
};

// can be extended
class SDFImp {
 public:
  SDFImp() : mesh(nullptr) {}

  /// get distance from nearest neighbor vertex.
  /// values are defined on cube corners.
  /// coordinate in origininal mesh model space.
  virtual float DistNN(Vec3f coord) { return 1e6f; }

  /// get distance using triliear interpolation. queries 8 vertices.
  /// coordinate in origininal mesh model space.
  virtual float DistTrilinear(Vec3f coord) { return 1e6f; }

  virtual void MarchingCubes(float level, TrigMesh* surf) {}

  virtual int Compute() { return 0; }

  SDFInfo conf;

  TrigMesh* mesh;
};

// signed distance function computed from a trig mesh.
struct SDFMesh {
  SDFMesh();
  /// SDFMesh takes ownership of the pointer.
  SDFMesh(SDFImp* impPtr);

  /// loses the original computed sdf if any.
  void SetImp(SDFImp* impPtr);

  /// get distance from nearest neighbor vertex.
  /// values are defined on cube corners.
  /// coordinate in origininal mesh model space.
  float DistNN(const Vec3f& coord);

  /// get distance using triliear interpolation. queries 8 vertices.
  /// coordinate in origininal mesh model space.
  float DistTrilinear(const Vec3f& coord);

  void MarchingCubes(float level, TrigMesh* surf);

  /// computes sdf given mesh and other conf.
  int Compute() { return imp->Compute(); }

  void SetVoxelSize(float h) { imp->conf.voxelSize = h; }
  float GetVoxelSize() const { return imp->conf.voxelSize; }

  void SetBand(float b) { imp->conf.band = b; }
  float GetBand() const { return imp->conf.band; }

  void SetMesh(TrigMesh* mesh) { imp->mesh = mesh; }

  Vec3f computeGradient(const Vec3f& coord);

 private:
  std::shared_ptr<SDFImp> imp;
};
