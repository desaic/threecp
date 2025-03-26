#pragma once

#include <mutex>

#include "ABuffer.h"
#include "Array2D.h"
#include "Array3D.h"
#include "Hit.h"
#include "RaycastConf.h"
#include "TrigMesh.h"
/// specialized to only cast rays along z.
/// Voxel (0,0,0) is centered at 0.5*voxel_size.
class ZRaycast {
 public:
  ZRaycast() : badRayCountFirstPass_(0), badRayCountFinal_(0) {}
  ~ZRaycast() {}

  int BadRayCountFirstPass() const { return badRayCountFirstPass_; }
  int BadRayCount() const { return badRayCountFinal_; }

  int Raycast(const TrigMesh& mesh, ABufferf& abuf, const RaycastConf& conf);

  int RaycastPixel(int x, int y, ABufferf::SegList& segs);
  int RaycastRow(unsigned row);

 private:
  int badRayCountFirstPass_ = 0;
  int badRayCountFinal_ = 0;
  std::mutex countLock_;

  // temporary states reset by every call to Raycast.
  RaycastConf conf_;
  // for each pixel, list of triangle indicies overlapping that pixel.
  Array2D<std::vector<unsigned> > coarseGrid_;
  const TrigMesh* mesh_ = nullptr;
  ABufferf* abuf_ = nullptr;
};

/// unintialized intervals have negative values.
/// valid intervals only have nonnegative values.
void UpdateInterval(Vec2i& interval, int x);

/// voxels are centered at (0.5,0.5) instead of (0,0).
/// only works for positive coordinates.
void RasterizeLine(Vec2f a, Vec2f b, std::vector<Vec2i>& intervals);
/// only works for positive coordinates.
void RasterizeTrig(const Vec2f* trig, std::vector<Vec2i>& row_intervals);
bool ZRayTrigInter(const Vec3f& v0, const Vec3f& e1, const Vec3f& e2, float det2x2, float ray_x,
                   float ray_y, float& z);
bool ZRayTrigInter(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2, float ray_x, float ray_y,
                   float& z);

/// <summary>
/// ZRaycast helper that can be unit tested.
/// Voxel (0,0,0) is centered at 0.5*voxel_size.
/// </summary>
/// <param name="mesh"></param>
/// <param name="grid">output. each cell contains list of intersecting triangle
/// ids.</param>
/// <param name="res">voxel size</param>
/// <param name="box">voxel grid bounding box</param>
void RasterizeXY(const TrigMesh& mesh, Array2D<std::vector<unsigned> >& grid, const Vec2f& res,
                 const BBox& box);

void RasterizeXYPar(unsigned rowStart, unsigned rowEnd, const std::vector<unsigned>& trigs,
                    const TrigMesh& mesh, Array2D<std::vector<unsigned> >& grid, const Vec2f& res,
                    const BBox& box);
void RasterizeXYSingle(const TrigMesh& mesh, Array2D<std::vector<unsigned> >& grid,
                       const Vec2f& res, const BBox& box);