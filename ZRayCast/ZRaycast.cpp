#include "ZRaycast.h"

#include <algorithm>
#include <iostream>

#include "Array2D.h"
#include "Array3D.h"
#include "MeshUtil.h"
#include "RasterizeTrig.h"
#include "ThreadPool.h"
#include "Stopwatch.h"
#include "Vec2.h"

#define MAX_GRID_SIZE 100000u
#define MAX_PIXELS 1000000000u

/// cast z rays for all pixels in a row.
/// results stored in Alpha-buffer.
class ZRaycastRow : public TPTask {
 public:
  ZRaycastRow(int row, ZRaycast& caster) : row_(row), rayCaster_(caster) {}

  virtual void run() override { rayCaster_.RaycastRow(row_); }

  virtual ~ZRaycastRow() override {}

 private:
  unsigned row_;
  ZRaycast& rayCaster_;
};

/// <summary>
///
/// </summary>
/// <param name="v0"></param>
/// <param name="e1">edge from v0 to v1</param>
/// <param name="e2">edge from v0 to v2</param>
/// <param name="det2x2"></param>
/// <param name="ray_x"></param>
/// <param name="ray_y"></param>
/// <returns>true if there is an intersection.</returns>
bool ZRayTrigInter(const Vec3f& v0, const Vec3f& e1, const Vec3f& e2, float det2x2, float ray_x,
                   float ray_y, float& z) {
  if (det2x2 == 0) {
    return false;
  }
  const float EPSILON = 1e-6;
  const float L_LOWERBOUND = -EPSILON;
  const float L_UPPERBOUND = 1 + EPSILON;
  Vec3f AO(ray_x - v0[0], ray_y - v0[1], -v0[2]);
  // compute barycentric coordinates and check their bounds.
  float lambda1 = (e2[1] * AO[0] - e2[0] * AO[1]) / det2x2;

  if (lambda1 < L_LOWERBOUND || lambda1 > L_UPPERBOUND) {
    return false;
  }
  float lambda2 = (-e1[1] * AO[0] + e1[0] * AO[1]) / det2x2;
  if (lambda2 < L_LOWERBOUND || lambda2 > L_UPPERBOUND) {
    return false;
  }
  float lambda3 = 1 - lambda1 - lambda2;
  if (lambda3 < L_LOWERBOUND) {
    return false;
  }
  z = v0[2] + lambda1 * e1[2] + lambda2 * e2[2];
  return true;
}

/// <summary>
///
/// </summary>
/// <param name="v0"></param>
/// <param name="v1"></param>
/// <param name="v2"></param>
/// <param name="ray_x"></param>
/// <param name="ray_y"></param>
/// <returns>true if there is an intersection.</returns>
bool ZRayTrigInter(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2, float ray_x, float ray_y,
                   float& z) {
  // Edge vectors
  Vec3f e0 = v1 - v0;
  Vec3f e1 = v2 - v1;
  Vec3f e2 = v2 - v0;
  float det2x2 = e0[0] * e2[1] - e0[1] * e2[0];
  return ZRayTrigInter(v0, e0, e2, det2x2, ray_x, ray_y, z);
}

int ZRaycastPoint(float x, float y, HitList& hits, const TrigMesh& mesh,
                  const Array2D<std::vector<unsigned> >& coarseGrid, const RaycastConf& conf) {
  const BBox& box = conf.box_;
  Vec2u gridSize = coarseGrid.GetSize();
  int coarseRow = int((y - box.vmin[1]) / conf.coarseRes_[1]);
  int coarseCol = int((x - box.vmin[0]) / conf.coarseRes_[0]);
  if (coarseRow < 0 || coarseRow >= gridSize[1] || coarseCol < 0 || coarseCol > gridSize[0]) {
    return -1;
  }
  float z0 = box.vmin[2];
  const std::vector<unsigned>& trigList = coarseGrid(coarseCol, coarseRow);
  for (unsigned i = 0; i < trigList.size(); i++) {
    unsigned tIdx = trigList[i];
    const Vec3f* v0 = (const Vec3f*)(&mesh.v[3 * mesh.t[3 * tIdx]]);
    const Vec3f* v1 = (const Vec3f*)(&mesh.v[3 * mesh.t[3 * tIdx + 1]]);
    const Vec3f* v2 = (const Vec3f*)(&mesh.v[3 * mesh.t[3 * tIdx + 2]]);
    float zIntersection = 0;
    bool inter = ZRayTrigInter(*v0, *v1, *v2, x, y, zIntersection);
    if (inter) {
      hits.push_back(Hit(zIntersection - z0, tIdx));
    }
  }

  return 0;
}

/// points have to be within the same coarse grid column
///  rays starts at (cx-dx,cy-dy),(cx+dx,cy-dy),(cx-dx,cy+dy),(cx+dx,cy+dy)
///@param cx center of the points
///@param dx
int ZRaycastPoints4(float cx, float cy, float dx, float dy, HitList* hitLists, const TrigMesh& mesh,
                    const Array2D<std::vector<unsigned> >& coarseGrid, const RaycastConf& conf) {
  Vec2u gridSize = coarseGrid.GetSize();
  const unsigned NUM_POINTS = 4;
  Vec2f points[NUM_POINTS] = {Vec2f(cx - dx, cy - dy), Vec2f(cx + dx, cy - dy),
                              Vec2f(cx - dx, cy + dy), Vec2f(cx + dx, cy + dy)};
  const BBox& box = conf.box_;
  float z0 = box.vmin[2];
  int coarseRow = int((cy - box.vmin[1]) / conf.coarseRes_[1]);
  int coarseCol = int((cx - box.vmin[0]) / conf.coarseRes_[0]);
  if (coarseRow < 0 || coarseRow >= gridSize[1] || coarseCol < 0 || coarseCol > gridSize[0]) {
    return -1;
  }
  const std::vector<unsigned>& trigList = coarseGrid(coarseCol, coarseRow);
  for (unsigned i = 0; i < trigList.size(); i++) {
    unsigned tIdx = trigList[i];
    const Vec3f* v0 = (const Vec3f*)(&mesh.v[3 * mesh.t[3 * tIdx]]);
    const Vec3f* v1 = (const Vec3f*)(&mesh.v[3 * mesh.t[3 * tIdx + 1]]);
    const Vec3f* v2 = (const Vec3f*)(&mesh.v[3 * mesh.t[3 * tIdx + 2]]);
    Vec3f e0 = *v1 - *v0;
    Vec3f e1 = *v2 - *v1;
    Vec3f e2 = *v2 - *v0;
    float det2x2 = e0[0] * e2[1] - e0[1] * e2[0];
    for (unsigned rayIndex = 0; rayIndex < NUM_POINTS; rayIndex++) {
      Vec2f origin = points[rayIndex];
      float zIntersection = 0;
      bool inter = ZRayTrigInter(*v0, e0, e2, det2x2, origin[0], origin[1], zIntersection);
      if (inter) {
        hitLists[rayIndex].push_back(Hit(zIntersection - z0, tIdx));
      }
    }
  }

  return 0;
}

int ZRaycast::RaycastPixel(int x, int y, ABufferf::SegList& segs) {
  float xcoord = conf_.box_.vmin[0] + (x + 0.5f) * conf_.voxelSize_[0];
  float ycoord = conf_.box_.vmin[1] + (y + 0.5f) * conf_.voxelSize_[1];
  float dx = 0.4f * conf_.voxelSize_[0];
  float dy = 0.4f * conf_.voxelSize_[1];
  // @TODO make configurable
  float zRes = abuf_->zRes;
  HitList hits;
  ZRaycastPoint(xcoord, ycoord, hits, *mesh_, coarseGrid_, conf_);

  size_t numHits = hits.size();
  int count = sortHits(hits, &(mesh_->nt));
  int ret = 0;
  // try casting 4 rays and pick the good ray if any.
  if (count != 0) {
    {
      std::lock_guard<std::mutex> lock(countLock_);
      badRayCountFirstPass_++;
    }
    const unsigned NUM_RAYS = 4;
    HitList hits4[NUM_RAYS];
    ret = ZRaycastPoints4(xcoord, ycoord, dx, dy, hits4, *mesh_, coarseGrid_, conf_);
    int bestRay = -1;
    int bestNumHits = -1;
    for (unsigned i = 0; i < NUM_RAYS; i++) {
      count = sortHits(hits4[i], &(mesh_->nt));
      if (count == 0) {
        // a candidate
        int numHits = int(hits4[i].size());
        // prefer good rays with more hits
        if (numHits > bestNumHits) {
          bestRay = int(i);
          bestNumHits = numHits;
        }
      }
    }

    if (bestRay < 0) {
      {
        std::lock_guard<std::mutex> lock(countLock_);
        badRayCountFinal_++;
      }
      return 0;
    } else {
      hits = hits4[bestRay];
    }
  }

  int err = ConvertHitsToSegs(hits, segs, zRes);
  if (err < 0) {
    ret = err;
  }
  return ret;
}

int ZRaycast::RaycastRow(unsigned row) {
  int ret = 0;
  if (abuf_ == nullptr || row >= abuf_->size[1]) {
    return -1;
  }
  unsigned x1 = abuf_->size[0];
  std::vector<ABufferf::SegList> slice(x1);
  for (unsigned x = 0; x < x1; x++) {
    int ret = RaycastPixel(int(x), row, slice[x]);
  }

  abuf_->GetRow(row).Compress(slice);
  return 0;
}

int ZRaycast::Raycast(const TrigMesh& mesh, ABufferf& abuf, const RaycastConf& conf) {
  conf_ = conf;
  abuf_ = &abuf;
  conf_.ComputeCoarseRes();
  // coarse grid and the final A-buffer share the same origin.
  Vec2f coarseRes(conf_.coarseRes_[0], conf_.coarseRes_[1]);
  Utils::Stopwatch timer;
  timer.Start();
  RasterizeXY(mesh, coarseGrid_, coarseRes, conf_.box_);
  
  mesh_ = &mesh;
  const BBox& bbox = conf_.box_;
  abuf_->resize(1, 1);
  badRayCountFirstPass_ = 0;
  badRayCountFinal_ = 0;
  Vec3u gridSize;
  for (unsigned d = 0; d < 3; d++) {
    gridSize[d] = unsigned((bbox.vmax[d] - bbox.vmin[d]) / conf_.voxelSize_[d]) + 1;
    if (gridSize[d] > MAX_GRID_SIZE) {
      return -1;
    }
  }
  if (bbox.vmax[2] - bbox.vmin[2] >= MAX_Z_HEIGHT_MM) {
    return -2;
  }
  size_t numPixels = gridSize[0] * size_t(gridSize[1]);
  if (numPixels > MAX_PIXELS) {
    return -3;
  }
  abuf_->resize(gridSize[0], gridSize[1]);
  ThreadPool tp;
  for (unsigned y = 0; y < gridSize[1]; y++) {
    std::shared_ptr<ZRaycastRow> task = std::make_shared<ZRaycastRow>(y, *this);
    tp.addTask(task);
  }
  timer.Start();
  tp.run();
  return 0;
}

void CopyTrigGrid2D(Array2D<std::vector<unsigned> >& grid, const Array2D8u& trig_grid,
                    const Vec2u& trig_min, unsigned ti) {
  Vec2u trig_grid_size = trig_grid.GetSize();
  Vec2u trig_max;
  Vec2u grid_size = grid.GetSize();

  for (unsigned d = 0; d < 2; d++) {
    trig_max[d] = trig_min[d] + trig_grid_size[d];
    if (trig_max[d] > grid_size[d]) {
      trig_max[d] = grid_size[d];
    }
  }

  for (unsigned y = trig_min[1]; y < trig_max[1]; y++) {
    unsigned ty = y - trig_min[1];
    for (unsigned x = trig_min[0]; x < trig_max[0]; x++) {
      unsigned tx = x - trig_min[0];
      if (trig_grid(tx, ty)) {
        grid(x, y).push_back(ti);
      }
    }
  }
}

/// cast z rays for all pixels in a row.
/// results stored in Alpha-buffer.
class RasterizeRowsTask : public TPTask {
 public:
  RasterizeRowsTask(unsigned row0, unsigned row1, const std::vector<unsigned>& trigs,
                    const TrigMesh& mesh, Array2D<std::vector<unsigned> >& grid, const Vec2f& res,
                    const BBox& box)
      : row0_(row0), row1_(row1), trigs_(trigs), mesh_(mesh), grid_(grid), res_(res), box_(box) {}

  virtual void run() override { RasterizeXYPar(row0_, row1_, trigs_, mesh_, grid_, res_, box_); }

  virtual ~RasterizeRowsTask() override {}

 private:
  const TrigMesh& mesh_;
  Array2D<std::vector<unsigned> >& grid_;
  const Vec2f& res_;
  const BBox& box_;
  unsigned row0_, row1_;
  const std::vector<unsigned>& trigs_;
};

void CopyTrigGrid2D(Array2D<std::vector<unsigned> >& grid, const std::vector<Vec2i>& intervals,
                    unsigned row0, unsigned ti) {
  Vec2u size = grid.GetSize();
  for (unsigned row = 0; row < intervals.size(); row++) {
    Vec2i range = intervals[row];
    if (range[0] < 0 || range[1] < 0) {
      continue;
    }
    if (range[1] >= int(size[0])) {
      range[1] = int(size[0]) - 1;
    }
    if (range[0] > range[1]) {
      range[0] = range[1];
    }
    for (unsigned x = range[0]; x <= range[1]; x++) {
      grid(x, row + row0).push_back(ti);
    }
  }
}

void CopyTrigGrid2D(Array2D<std::vector<unsigned> >& grid, const std::vector<Vec2i>& intervals,
                    unsigned row0, unsigned rowStart, unsigned rowEnd, unsigned ti) {
  Vec2u size = grid.GetSize();
  for (unsigned row = 0; row < intervals.size(); row++) {
    unsigned dstRow = row + row0;
    if (dstRow < rowStart) {
      continue;
    }
    if (dstRow > rowEnd) {
      break;
    }
    Vec2i range = intervals[row];
    if (range[0] < 0 || range[1] < 0) {
      continue;
    }
    if (range[1] >= int(size[0])) {
      range[1] = int(size[0]) - 1;
    }
    if (range[0] > range[1]) {
      range[0] = range[1];
    }
    for (unsigned x = range[0]; x <= range[1]; x++) {
      grid(x, dstRow).push_back(ti);
    }
  }
}

void BBox2D(Vec2f& mn, Vec2f& mx, const Vec2f* verts, size_t nV) {
  mn[0] = verts[0][0];
  mn[1] = verts[0][1];
  mx = mn;
  for (size_t i = 1; i < nV; i++) {
    mn[0] = std::min(verts[i][0], mn[0]);
    mn[1] = std::min(verts[i][1], mn[1]);
    mx[0] = std::max(verts[i][0], mx[0]);
    mx[1] = std::max(verts[i][1], mx[1]);
  }
}

void YRange(Vec2f& yrange, const Vec2f* verts, size_t nV) {
  yrange[0] = verts[0][1];
  yrange[1] = verts[0][1];
  for (size_t i = 1; i < nV; i++) {
    yrange[0] = std::min(verts[i][1], yrange[0]);
    yrange[1] = std::max(verts[i][1], yrange[1]);
  }
}

void RasterizeXYPar(unsigned rowStart, unsigned rowEnd, const std::vector<unsigned>& trigs,
                    const TrigMesh& mesh, Array2D<std::vector<unsigned> >& grid, const Vec2f& res,
                    const BBox& box) {
  Vec3f gridOrigin = box.vmin;
  size_t nTrigs = mesh.t.size() / 3;
  if (rowEnd < rowStart) {
    return;
  }
  Vec2u size = grid.GetSize();
  Vec2f trig2D[3];
  // min grid index for triangle.
  Vec2f yrange;
  float invResX = 1.0f / res[0];
  float invResY = 1.0f / res[1];
  for (size_t i = 0; i < trigs.size(); i++) {
    unsigned ti = trigs[i];
    trig2D[0] = *(Vec2f*)(&mesh.v[3 * mesh.t[3 * ti]]);
    trig2D[1] = *(Vec2f*)(&mesh.v[3 * mesh.t[3 * ti + 1]]);
    trig2D[2] = *(Vec2f*)(&mesh.v[3 * mesh.t[3 * ti + 2]]);
    YRange(yrange, trig2D, 3);
    int row0 = std::max(0, int((yrange[0] - box.vmin[1]) * invResY));
    float y0 = box.vmin[1] + row0 * res[1];
    for (unsigned t = 0; t < 3; t++) {
      trig2D[t][0] = (trig2D[t][0] - box.vmin[0]) * invResX;
      trig2D[t][1] = (trig2D[t][1] - y0) * invResY;
    }
    unsigned numRows = unsigned((yrange[1] - y0) * invResY) + 1;

    // rasterize a triangle within its bounding box.
    std::vector<Vec2i> intervals(numRows, Vec2i(-1, -1));
    RasterizeTrig(trig2D, intervals);
    CopyTrigGrid2D(grid, intervals, row0, rowStart, rowEnd, ti);
  }
}

void RasterizeXYSeq(unsigned t0, unsigned t1, const TrigMesh& mesh,
                    Array2D<std::vector<unsigned> >& grid, const Vec2f& res, const BBox& box) {
  Vec3f gridOrigin = box.vmin;
  size_t nTrigs = mesh.t.size() / 3;
  t1 = std::min(unsigned(nTrigs), t1);
  if (t1 < t0) {
    return;
  }
  Vec2u size = grid.GetSize();
  Array2D<std::vector<unsigned> > localGrid(size[0], size[1]);
  Vec2f trig2D[3];
  // min grid index for triangle.
  Vec2f yrange;
  float invResX = 1.0f / res[0];
  float invResY = 1.0f / res[1];
  for (size_t ti = t0; ti <= t1; ti++) {
    trig2D[0] = *(Vec2f*)(&mesh.v[3 * mesh.t[3 * ti]]);
    trig2D[1] = *(Vec2f*)(&mesh.v[3 * mesh.t[3 * ti + 1]]);
    trig2D[2] = *(Vec2f*)(&mesh.v[3 * mesh.t[3 * ti + 2]]);
    YRange(yrange, trig2D, 3);
    int row0 = std::max(0, int((yrange[0] - box.vmin[1]) * invResY));
    int row1 = std::max(row0, int((yrange[1] - box.vmin[1]) * invResY));
    float y0 = box.vmin[1] + row0 * res[1];
    unsigned numRows = unsigned(row1 - row0 + 1);
    for (unsigned t = 0; t < 3; t++) {
      trig2D[t][0] = (trig2D[t][0] - box.vmin[0]) * invResX;
      trig2D[t][1] = (trig2D[t][1] - y0) * invResY;
    }
    // rasterize a triangle within its bounding box.
    std::vector<Vec2i> intervals(numRows, Vec2i(-1, -1));
    RasterizeTrig(trig2D, intervals);
    CopyTrigGrid2D(grid, intervals, row0, ti);
  }
}

void RasterizeXY(const TrigMesh& mesh, Array2D<std::vector<unsigned> >& grid, const Vec2f& res,
                 const BBox& box) {
  Vec3f gridOrigin = box.vmin;
  size_t nTrigs = mesh.t.size() / 3;

  Vec3f gridLen = box.vmax - box.vmin;
  Vec2u gridSize;
  gridSize[0] = unsigned((gridLen[0] / res[0]) + 1);
  gridSize[1] = unsigned((gridLen[1] / res[1]) + 1);

  grid.Allocate(gridSize[0], gridSize[1]);
  if (nTrigs == 0) {
    return;
  }

  // each thread does 50 rows a time.
  const unsigned BATCH_ROWS = 50;
  unsigned numBatches = gridSize[1] / BATCH_ROWS + ((gridSize[1] % BATCH_ROWS) > 0);

  // find all triangles for each batch.
  std::mutex gridLock;
  std::vector<std::vector<unsigned> > trigs(numBatches);
  for (size_t t = 0; t < nTrigs; t++) {
    float y = mesh.v[3 * mesh.t[3 * t] + 1];
    float ymin = y;
    float ymax = y;
    for (size_t j = 1; j < 3; j++) {
      float y = mesh.v[3 * mesh.t[3 * t + j] + 1];
      ymin = std::min(y, ymin);
      ymax = std::max(y, ymax);
    }

    unsigned row0 = unsigned((ymin - gridOrigin[1]) / res[1]);
    unsigned row1 = unsigned((ymax - gridOrigin[1]) / res[1]);
    unsigned b0 = row0 / BATCH_ROWS;
    unsigned b1 = row1 / BATCH_ROWS;
    b1 = std::min(b1, unsigned(trigs.size() - 1));
    b0 = std::min(b0, b1);
    for (unsigned b = b0; b <= b1; b++) {
      trigs[b].push_back(t);
    }
  }

  ThreadPool threadPool;
  // threadPool.setUserNumThreads(1);
  for (unsigned batch = 0; batch < numBatches; batch++) {
    unsigned row0 = batch * BATCH_ROWS;
    unsigned row1 = std::min(unsigned(gridSize[1] - 1), row0 + BATCH_ROWS - 1);
    std::shared_ptr<RasterizeRowsTask> task =
        std::make_shared<RasterizeRowsTask>(row0, row1, trigs[batch], mesh, grid, res, box);
    threadPool.addTask(task);
  }
  threadPool.run();
}

void RasterizeXYSingle(const TrigMesh& mesh, Array2D<std::vector<unsigned> >& grid,
                       const Vec2f& res, const BBox& box) {
  Vec3f gridOrigin = box.vmin;
  size_t nTrigs = mesh.t.size() / 3;

  Vec3f gridLen = box.vmax - box.vmin;
  Vec2u gridSize;
  gridSize[0] = unsigned((gridLen[0] / res[0]) + 1);
  gridSize[1] = unsigned((gridLen[1] / res[1]) + 1);

  grid.Allocate(gridSize[0], gridSize[1]);
  if (nTrigs == 0) {
    return;
  }
  RasterizeXYSeq(0, nTrigs - 1, mesh, grid, res, box);
}
