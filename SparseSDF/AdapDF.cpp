#include "AdapDF.h"

#include <iostream>

#include "BBox.h"
#include "FastSweep.h"
#include "ImageIO.h"
#include "MarchingCubes.h"
#include "Array2d.h"
#include "SDFVoxCb.h"
#include "ThreadPool.h"
#include "TrigMesh.h"
#include "Stopwatch.h"
#include "cpu_voxelizer.h"

AdapDF::AdapDF() { fineGrid.resize(1); }

int AdapDF::Allocate(unsigned sx, unsigned sy, unsigned sz) {
  size_t numVox = sx * sy * size_t(sz);
  if (numVox > MAX_NUM_VOX) {
    return -1;
  }
  // 1 more vertex than voxels.
  dist.Allocate(sx + 1, sy + 1, sz + 1);
  dist.Fill(MAX_DIST);

  Vec3u coarseSize(sx / 4, sy / 4, sz / 4);
  coarseSize[0] += (sx % 4 > 0);
  coarseSize[1] += (sy % 4 > 0);
  coarseSize[2] += (sz % 4 > 0);

  sparseGrid.Allocate(coarseSize[0], coarseSize[1], coarseSize[2]);
  trigList.resize(1);
  return 0;
}

unsigned AdapDF::AddDenseCell(const Vec3u& gridIdx) {
  unsigned cellIdx = sparseGrid.AddDense(gridIdx[0], gridIdx[1], gridIdx[2]);
  return cellIdx;
}

void AdapDF::BuildTrigList(TrigMesh* mesh) {
  mesh_ = mesh;
  voxconf conf;
  conf.unit = Vec3f(voxSize, voxSize, voxSize);

  BBox box;
  ComputeBBox(mesh->v, box);
  // add margin for narrow band sdf
  band = std::min(band, AdapDF::MAX_BAND);
  box.vmin = box.vmin - float(band) * conf.unit;
  box.vmax = box.vmax + float(band) * conf.unit;
  origin = box.vmin;
  conf.origin = origin;

  ClearTemporaryArrays();

  Vec3f count = (box.vmax - box.vmin) / conf.unit;
  conf.gridSize = Vec3u(count[0] + 1, count[1] + 1, count[2] + 1);
  Allocate(count[0], count[1], count[2]);

  TrigListVoxCb cb;
  cb.df = this;
  cb.m = mesh;
  Utils::Stopwatch timer;
  timer.Start();
  cpu_voxelize_mesh(conf, mesh, cb);
  float ms = timer.ElapsedMS();
}

class CoarseTrigTask : public TPTask {
 public:
  virtual void run() {
    if (_df == nullptr) {
      return;
    }
    Vec3u size = _df->dist.GetSize();
    _z1 = std::min(size[2] - 1, _z1);
    // loop through fine cells.
    // number of cells is 1 - number of vertices and dist is stored on vertices.
    for (unsigned z = _z0; z < _z1; z++) {
      for (unsigned y = 0; y < size[1]; y++) {
        for (unsigned x = 0; x < size[0]; x++) {
          _df->ComputeCoarseDist(x, y, z);
        }
      }
    }
  }
  CoarseTrigTask(AdapDF* df, unsigned z0, unsigned z1) : _df(df), _z0(z0), _z1(z1) {}
  virtual ~CoarseTrigTask() {}

 private:
  AdapDF* _df = nullptr;
  unsigned _z0 = 0, _z1 = 0;
};

void AdapDF::ComputeCoarseDist() {
  if (mesh_ == nullptr) {
    return;
  }
  size_t numTrigs = mesh_->t.size() / 3;
  trigFrames.resize(numTrigs);
  for (size_t i = 0; i < numTrigs; i++) {
    Triangle trig = mesh_->GetTriangleVerts(i);
    Vec3f n = mesh_->GetTrigNormal(i);
    ComputeTrigFrame((const float*)(&trig.v), n, trigFrames[i]);
  }

  // loop through coarse vertices
  Vec3u size = dist.GetSize();
  ThreadPool tp;
  const unsigned zBatchSize = 16;
  const unsigned numBatches = (size[2] - 1) / zBatchSize + 1;
  for (unsigned zBatch = 0; zBatch < numBatches; zBatch++) {
    unsigned z0 = zBatch * zBatchSize;
    unsigned z1 = z0 + zBatchSize;
    z1 = std::min(size[2] - 1, z1);
    std::shared_ptr<CoarseTrigTask> task = std::make_shared<CoarseTrigTask>(this, z0, z1);
    tp.addTask(task);
  }
  tp.run();
}

static bool IsCorner(unsigned x, unsigned y, unsigned z, unsigned N) {
  return (x == 0 || x == N - 1) && (y == 0 || y == N - 1) && (z == 0 || z == N - 1);
}

void AdapDF::ComputeDistGrid5(Vec3f x0, FixedGrid3D<5>& fine, const std::vector<size_t>& trigs) {
  const unsigned N = fine.N;
  // fine voxel size
  float h = voxSize / float(N - 1);
  for (unsigned z = 0; z < N; z++) {
    for (unsigned y = 0; y < N; y++) {
      for (unsigned x = 0; x < N; x++) {
        if (IsCorner(x, y, z, N)) {
          // already computed and frozen.
          continue;
        }
        Vec3f point = x0 + h * Vec3f(x, y, z);
        float d = MinDist(trigs, trigFrames, point, mesh_);
        fine(x, y, z) = short(d / distUnit);
      }
    }
  }
}

class SweepTask : public TPTask {
 public:
  virtual void run() {
    if (_df == nullptr) {
      return;
    }
    for (size_t i = _i0; i < _i1; i++) {
      _df->ComputeFineCellDist(i);
    }
  }

  void SetArgs(AdapDF* df, size_t i0, size_t i1) {
    _df = df;
    _i0 = i0;
    _i1 = i1;
  }
  SweepTask() {}
  virtual ~SweepTask() {}

 private:
  AdapDF* _df = nullptr;
  size_t _i0 = 0, _i1 = 0;
};

class PtTrigTask : public TPTask {
 public:
  virtual void run() {
    if (_df == nullptr) {
      return;
    }
    Vec3u dsize = _df->dist.GetSize();
    _z1 = std::min(dsize[2] - 1, _z1);
    // loop through fine cells.
    // number of cells is 1 - number of vertices and dist is stored on vertices.
    for (unsigned z = _z0; z < _z1; z++) {
      for (unsigned y = 0; y < dsize[1] - 1; y++) {
        for (unsigned x = 0; x < dsize[0] - 1; x++) {
          _df->ComputeFinePtTrig(x, y, z);
        }
      }
    }
  }
  PtTrigTask(AdapDF* df, unsigned z0, unsigned z1) : _df(df), _z0(z0), _z1(z1) {}
  virtual ~PtTrigTask() {}

 private:
  AdapDF* _df = nullptr;
  unsigned _z0 = 0, _z1 = 0;
};

void AdapDF::ComputeFineDistBrute() {
  // allocate fine cells.
  fineGrid.resize(trigList.size());
  Vec3u dsize = dist.GetSize();
  const unsigned N = FixedGrid3D<5>::N;
  Utils::Stopwatch timer;
  timer.Start();
  ThreadPool tp;
  const unsigned zBatchSize = 10;
  const unsigned numBatches = (dsize[2] - 1) / zBatchSize + 1;
  for (unsigned zBatch = 0; zBatch < numBatches; zBatch++) {
    unsigned z0 = zBatch * zBatchSize;
    unsigned z1 = z0 + zBatchSize;
    z1 = std::min(dsize[2] - 1, z1);
    std::shared_ptr<PtTrigTask> task = std::make_shared<PtTrigTask>(this, z0, z1);
    tp.addTask(task);
  }
  tp.run();
  float ms = timer.ElapsedMS();
  timer.Start();
  PropagateNeighbors();
  ms = timer.ElapsedMS();
  // fast sweep, freeze corner cells and cells within h_fine.
  timer.Start();
  FastSweepFine();
  ms = timer.ElapsedMS();
}

void AdapDF::FastSweepCoarse(Array3D<uint8_t>& frozen) {
  FastSweepPar(dist, voxSize, distUnit, band, frozen);
}

void AdapDF::PropagateNeighbors() {
  Vec3u dsize = dist.GetSize();
  if (fineGrid.empty()) {
    return;
  }
  const unsigned N = fineGrid[0].N;
  // take min of edge vertices from diagonal neighbors,
  // if a neighbor cell has no fine grid, skip it.
  // x
  for (unsigned z = 0; z < dsize[2] - 2; z++) {
    for (unsigned y = 0; y < dsize[1] - 2; y++) {
      for (unsigned x = 0; x < dsize[0] - 1; x++) {
        unsigned sparseIdx1 = GetSparseCellIdx(x, y, z);
        unsigned sparseIdx2 = GetSparseCellIdx(x, y + 1, z + 1);
        // compute min only if both adjace cells are present.
        if (sparseIdx1 == 0 || sparseIdx2 == 0) {
          continue;
        }
        FixedGrid3D<5>& fine1 = fineGrid[sparseIdx1];
        FixedGrid3D<5>& fine2 = fineGrid[sparseIdx2];
        for (unsigned finex = 1; finex < N - 1; finex++) {
          short val1 = fine1(finex, N - 1, N - 1);
          short val2 = fine2(finex, 0, 0);
          if (std::abs(val1) > std::abs(val2)) {
            fine1(finex, N - 1, N - 1) = val2;
          } else {
            fine2(finex, 0, 0) = val1;
          }
        }
      }
    }
  }
  // y
  for (unsigned z = 0; z < dsize[2] - 2; z++) {
    for (unsigned y = 0; y < dsize[1] - 1; y++) {
      for (unsigned x = 0; x < dsize[0] - 2; x++) {
        unsigned sparseIdx1 = GetSparseCellIdx(x, y, z);
        unsigned sparseIdx2 = GetSparseCellIdx(x + 1, y, z + 1);
        // compute min only if both adjace cells are present.
        if (sparseIdx1 == 0 || sparseIdx2 == 0) {
          continue;
        }
        FixedGrid3D<5>& fine1 = fineGrid[sparseIdx1];
        FixedGrid3D<5>& fine2 = fineGrid[sparseIdx2];
        for (unsigned finey = 1; finey < N - 1; finey++) {
          short val1 = fine1(N - 1, finey, N - 1);
          short val2 = fine2(0, finey, 0);
          if (std::abs(val1) > std::abs(val2)) {
            fine1(N - 1, finey, N - 1) = val2;
          } else {
            fine2(0, finey, 0) = val1;
          }
        }
      }
    }
  }
  // z
  for (unsigned z = 0; z < dsize[2] - 1; z++) {
    for (unsigned y = 0; y < dsize[1] - 2; y++) {
      for (unsigned x = 0; x < dsize[0] - 2; x++) {
        unsigned sparseIdx1 = GetSparseCellIdx(x, y, z);
        unsigned sparseIdx2 = GetSparseCellIdx(x + 1, y + 1, z);
        // compute min only if both adjace cells are present.
        if (sparseIdx1 == 0 || sparseIdx2 == 0) {
          continue;
        }
        FixedGrid3D<5>& fine1 = fineGrid[sparseIdx1];
        FixedGrid3D<5>& fine2 = fineGrid[sparseIdx2];
        for (unsigned finez = 1; finez < N - 1; finez++) {
          short val1 = fine1(N - 1, N - 1, finez);
          short val2 = fine2(0, 0, finez);
          if (std::abs(val1) > std::abs(val2)) {
            fine1(N - 1, N - 1, finez) = val2;
          } else {
            fine2(0, 0, finez) = val1;
          }
        }
      }
    }
  }

  // take min of face vertices
  // yz
  for (unsigned z = 0; z < dsize[2] - 1; z++) {
    for (unsigned y = 0; y < dsize[1] - 1; y++) {
      for (unsigned x = 0; x < dsize[0] - 2; x++) {
        unsigned sparseIdx1 = GetSparseCellIdx(x, y, z);
        unsigned sparseIdx2 = GetSparseCellIdx(x + 1, y, z);
        // compute min only if both adjace cells are present.
        if (sparseIdx1 == 0 || sparseIdx2 == 0) {
          continue;
        }
        FixedGrid3D<5>& fine1 = fineGrid[sparseIdx1];
        FixedGrid3D<5>& fine2 = fineGrid[sparseIdx2];
        for (unsigned finez = 0; finez < N; finez++) {
          for (unsigned finey = 0; finey < N; finey++) {
            short val1 = fine1(N - 1, finey, finez);
            short val2 = fine2(0, finey, finez);
            if (std::abs(val1) > std::abs(val2)) {
              fine1(N - 1, finey, finez) = val2;
            } else {
              fine2(0, finey, finez) = val1;
            }
          }
        }
      }
    }
  }
  // xz
  for (unsigned z = 0; z < dsize[2] - 1; z++) {
    for (unsigned y = 0; y < dsize[1] - 2; y++) {
      for (unsigned x = 0; x < dsize[0] - 1; x++) {
        unsigned sparseIdx1 = GetSparseCellIdx(x, y, z);
        unsigned sparseIdx2 = GetSparseCellIdx(x, y + 1, z);
        // compute min only if both adjace cells are present.
        if (sparseIdx1 == 0 || sparseIdx2 == 0) {
          continue;
        }
        FixedGrid3D<5>& fine1 = fineGrid[sparseIdx1];
        FixedGrid3D<5>& fine2 = fineGrid[sparseIdx2];
        for (unsigned finez = 0; finez < N; finez++) {
          for (unsigned finex = 0; finex < N; finex++) {
            short val1 = fine1(finex, N - 1, finez);
            short val2 = fine2(finex, 0, finez);
            if (std::abs(val1) > std::abs(val2)) {
              fine1(finex, N - 1, finez) = val2;
            } else {
              fine2(finex, 0, finez) = val1;
            }
          }
        }
      }
    }
  }
  // xy
  for (unsigned z = 0; z < dsize[2] - 2; z++) {
    for (unsigned y = 0; y < dsize[1] - 1; y++) {
      for (unsigned x = 0; x < dsize[0] - 1; x++) {
        unsigned sparseIdx1 = GetSparseCellIdx(x, y, z);
        unsigned sparseIdx2 = GetSparseCellIdx(x, y, z + 1);
        // compute min only if both adjace cells are present.
        if (sparseIdx1 == 0 || sparseIdx2 == 0) {
          continue;
        }
        FixedGrid3D<5>& fine1 = fineGrid[sparseIdx1];
        FixedGrid3D<5>& fine2 = fineGrid[sparseIdx2];

        for (unsigned finey = 0; finey < N; finey++) {
          for (unsigned finex = 0; finex < N; finex++) {
            short val1 = fine1(finex, finey, N - 1);
            short val2 = fine2(finex, finey, 0);
            if (std::abs(val1) > std::abs(val2)) {
              fine1(finex, finey, N - 1) = val2;
            } else {
              fine2(finex, finey, 0) = val1;
            }
          }
        }
      }
    }
  }
}

void AdapDF::FastSweepFine() {
  ThreadPool tp;
  const size_t batchSize = 1000;
  size_t numBatches = fineGrid.size() / batchSize + 1;
  for (size_t batch = 0; batch < numBatches; batch++) {
    // 0 reserved for empty coarse cells.
    size_t i0 = 1 + batchSize * batch;
    size_t i1 = std::min(i0 + batchSize, fineGrid.size());
    std::shared_ptr<SweepTask> task = std::make_shared<SweepTask>();
    task->SetArgs(this, i0, i1);
    tp.addTask(task);
  }
  tp.run();
}

void AdapDF::ComputeFineCellDist(size_t sparseIdx) {
  FixedGrid3D<5>& fine = fineGrid[sparseIdx];
  const unsigned N = fine.N;
  Array3D8u frozen;
  frozen.Allocate(Vec3u(N, N, N), 0);
  frozen(0, 0, 0) = 1;
  frozen(N - 1, 0, 0) = 1;
  frozen(0, N - 1, 0) = 1;
  frozen(N - 1, N - 1, 0) = 1;
  frozen(0, 0, N - 1) = 1;
  frozen(N - 1, 0, N - 1) = 1;
  frozen(0, N - 1, N - 1) = 1;
  frozen(N - 1, N - 1, N - 1) = 1;
  float h_fine = voxSize / (N - 1);
  FastSweep(fine, h_fine, distUnit, band * N, frozen);
}

void AdapDF::ComputeFinePtTrig(unsigned x, unsigned y, unsigned z) {
  unsigned sparseIdx = GetSparseCellIdx(x, y, z);
  // index 0 reserved for emtpy cells.
  if (sparseIdx == 0) {
    return;
  }
  FixedGrid3D<5>& fine = fineGrid[sparseIdx];
  const unsigned N = fine.N;
  // corner vertices
  fine(0, 0, 0) = dist(x, y, z);
  fine(N - 1, 0, 0) = dist(x + 1, y, z);
  fine(0, N - 1, 0) = dist(x, y + 1, z);
  fine(N - 1, N - 1, 0) = dist(x + 1, y + 1, z);
  fine(0, 0, N - 1) = dist(x, y, z + 1);
  fine(N - 1, 0, N - 1) = dist(x + 1, y, z + 1);
  fine(0, N - 1, N - 1) = dist(x, y + 1, z + 1);
  fine(N - 1, N - 1, N - 1) = dist(x + 1, y + 1, z + 1);

  // find closest distance within the coarse cell
  const std::vector<size_t>& trigs = trigList[sparseIdx];
  if (trigs.size() == 0) {
    return;
  }
  Vec3f x0 = voxSize * Vec3f(x, y, z) + origin;
  ComputeDistGrid5(x0, fine, trigs);
}

void AdapDF::GatherTrigs(unsigned x, unsigned y, unsigned z, std::vector<size_t>& trigs) {
  Vec3u size = sparseGrid.GetFineSize();
  if (x >= size[0] || y >= size[1] || z >= size[2]) {
    return;
  }
  unsigned idx = GetSparseCellIdx(x, y, z);
  if (idx == 0) {
    // special index empty cell
    return;
  }
  const std::vector<size_t>& cell = trigList[idx];
  trigs.insert(trigs.end(), cell.begin(), cell.end());
}

void AdapDF::ComputeCoarseDist(unsigned x, unsigned y, unsigned z) {
  // gather triangles adjacent to this vertex.
  // using set or unordered_set is slower due to overhead.
  std::vector<size_t> trigs;
  GatherTrigs(x - 1, y - 1, z - 1, trigs);
  GatherTrigs(x, y - 1, z - 1, trigs);
  GatherTrigs(x - 1, y, z - 1, trigs);
  GatherTrigs(x, y, z - 1, trigs);
  GatherTrigs(x - 1, y - 1, z, trigs);
  GatherTrigs(x, y - 1, z, trigs);
  GatherTrigs(x - 1, y, z, trigs);
  GatherTrigs(x, y, z, trigs);
  if (trigs.size() == 0) {
    return;
  }
  Vec3f point = voxSize * Vec3f(x, y, z) + origin;
  float minDist = MinDist(trigs, trigFrames, point, mesh_);
  dist(x, y, z) = short(minDist / distUnit);
}

bool AdapDF::HasCellDense(const Vec3u& gridIdx) const {
  return sparseGrid.HasDense(gridIdx[0], gridIdx[1], gridIdx[2]);
}

bool AdapDF::HasCellSparse(const Vec3u& gridIdx) const {
  return sparseGrid.HasSparse(gridIdx[0], gridIdx[1], gridIdx[2]);
}

void AdapDF::Compress() { sparseGrid.Compress(); }

void AdapDF::ClearTemporaryArrays() {
  trigList.clear();
  trigFrames.clear();
}

// if any corner distance is uninitialized
static bool OutsideNarrowBand(const Array3D<short>& dist, unsigned ix, unsigned iy, unsigned iz) {
  short corners[8];
  corners[0] = dist(ix, iy, iz);
  corners[1] = dist(ix + 1, iy, iz);
  corners[2] = dist(ix, iy + 1, iz);
  corners[3] = dist(ix + 1, iy + 1, iz);

  corners[4] = dist(ix, iy, iz + 1);
  corners[5] = dist(ix + 1, iy, iz + 1);
  corners[6] = dist(ix, iy + 1, iz + 1);
  corners[7] = dist(ix + 1, iy + 1, iz + 1);
  for (int i = 0; i < 8; i++) {
    if (corners[i] >= AdapDF::MAX_DIST - 1) {
      return true;
    }
  }
  return false;
}

Vec3f AdapDF::WorldCoord(const Vec3f& gridCoord) const {
  return voxSize * gridCoord + origin;
}

/// <summary>
/// Triliearly interpolates the vertex distance grid.
/// </summary>
/// <returns>distance in mm</returns>
float AdapDF::GetCoarseDist(const Vec3f& x) const {
  Vec3f local = x - origin;
  local = local / voxSize;
  unsigned ix = unsigned(local[0]);
  unsigned iy = unsigned(local[1]);
  unsigned iz = unsigned(local[2]);
  Vec3u gridSize = dist.GetSize();
  if (ix >= gridSize[0] - 1 || iy >= gridSize[1] - 1 || iz >= gridSize[2] - 1) {
    return MAX_DIST;
  }
  if (OutsideNarrowBand(dist, ix, iy, iz)) {
    return MAX_DIST;
  }
  float a = (ix + 1) - local[0];
  float b = (iy + 1) - local[1];
  float c = (iz + 1) - local[2];
  // v indexed by y and z
  float v00 = a * dist(ix, iy, iz) + (1 - a) * dist(ix + 1, iy, iz);
  float v10 = a * dist(ix, iy + 1, iz) + (1 - a) * dist(ix + 1, iy + 1, iz);
  float v01 = a * dist(ix, iy, iz + 1) + (1 - a) * dist(ix + 1, iy, iz + 1);
  float v11 = a * dist(ix, iy + 1, iz + 1) + (1 - a) * dist(ix + 1, iy + 1, iz + 1);
  float v0 = b * v00 + (1 - b) * v10;
  float v1 = b * v01 + (1 - b) * v11;
  float v = c * v0 + (1 - c) * v1;
  return v * distUnit;
}
/// <summary>
/// taken as the min of the bilinear interpolated distance
/// and the point sample based distance.
/// </summary>
/// <returns>distance in mm</returns>
float AdapDF::GetFineDist(const Vec3f& x) const {
  Vec3f local = x - origin;
  Vec3f idx = local / voxSize;
  unsigned ix = unsigned(idx[0]);
  unsigned iy = unsigned(idx[1]);
  unsigned iz = unsigned(idx[2]);
  Vec3u gridSize = dist.GetSize();
  if (ix >= gridSize[0] - 1 || iy >= gridSize[1] - 1 || iz >= gridSize[2] - 1) {
    return MAX_DIST;
  }

  const SparseNode4<unsigned>& node = GetSparseNode4(ix, iy, iz);
  float coarseDist = GetCoarseDist(x);
  if (!node.HasChild(ix & 3, iy & 3, iz & 3)) {
    return coarseDist;
  }
  unsigned nodeIdx = *(node.GetChild(ix & 3, iy & 3, iz & 3));
  float fineDist = MAX_DIST;
  const FixedGrid3D<5>& fGrid = fineGrid[nodeIdx];

  Vec3f fineCoord = local - voxSize * Vec3f(ix, iy, iz);
  const unsigned N = FixedGrid3D<5>::N;
  float fine_h = voxSize / (N - 1);

  float indexEps = 1e-3;
  Vec3f fineIdx = (1.0f / fine_h) * fineCoord;
  // handle case where point is on right wall etc.
  for (unsigned d = 0; d < 3; d++) {
    fineIdx[d] = std::min(fineIdx[d], float(N - 1));
  }
  unsigned fx = unsigned(fineIdx[0] - indexEps);
  unsigned fy = unsigned(fineIdx[1] - indexEps);
  unsigned fz = unsigned(fineIdx[2] - indexEps);
  float a = (fx + 1) - fineIdx[0];
  float b = (fy + 1) - fineIdx[1];
  float c = (fz + 1) - fineIdx[2];
  float v00 = a * fGrid(fx, fy, fz) + (1 - a) * fGrid(fx + 1, fy, fz);
  float v10 = a * fGrid(fx, fy + 1, fz) + (1 - a) * fGrid(fx + 1, fy + 1, fz);
  float v01 = a * fGrid(fx, fy, fz + 1) + (1 - a) * fGrid(fx + 1, fy, fz + 1);
  float v11 = a * fGrid(fx, fy + 1, fz + 1) + (1 - a) * fGrid(fx + 1, fy + 1, fz + 1);
  float v0 = b * v00 + (1 - b) * v10;
  float v1 = b * v01 + (1 - b) * v11;
  float v = c * v0 + (1 - c) * v1;
  v *= distUnit;
  return v;
}

SparseNode4<unsigned>& AdapDF::GetSparseNode4(unsigned x, unsigned y, unsigned z) {
  return sparseGrid.GetSparseNode4(x, y, z);
}

const SparseNode4<unsigned>& AdapDF::GetSparseNode4(unsigned x, unsigned y, unsigned z) const {
  return sparseGrid.GetSparseNode4(x, y, z);
}

unsigned AdapDF::GetSparseCellIdx(unsigned x, unsigned y, unsigned z) const {
  const SparseNode4<unsigned>& node = GetSparseNode4(x, y, z);
  if (!node.HasChild(x & 3, y & 3, z & 3)) {
    return 0;
  }
  unsigned nodeIdx = *(node.GetChild(x & 3, y & 3, z & 3));
  return nodeIdx;
}

int SDFImpAdap::Compute() {
  _df->voxSize = conf.voxelSize;
  _df->band = conf.band;
  _df->ClearTemporaryArrays();
  Utils::Stopwatch timer;
  timer.Start();
  _df->BuildTrigList(mesh);
  float ms = timer.ElapsedMS();
  timer.Start();
  _df->Compress();
  ms = timer.ElapsedMS();
  timer.Start();
  mesh->ComputePseudoNormals();
  ms = timer.ElapsedMS();
  timer.Start();
  _df->ComputeCoarseDist();
  ms = timer.ElapsedMS();
  timer.Start();
  CloseExterior(_df->dist, _df->MAX_DIST);
  ms = timer.ElapsedMS();
  Array3D8u frozen;
  timer.Start();
  _df->FastSweepCoarse(frozen);

  ms = timer.ElapsedMS();
  _df->ComputeFineDistBrute();
  _df->ClearTemporaryArrays();
  return 0;
}

float SDFImpAdap::DistNN(Vec3f coord) {
  float dist = _df->GetCoarseDist(coord);
  return dist;
}

float SDFImpAdap::DistTrilinear(Vec3f coord) {
  float dist = _df->GetFineDist(coord);
  return dist;
}

void SDFImpAdap::MarchingCubes(float level, TrigMesh* surf) {
  const Array3D<short>& grid = _df->dist;
  Vec3u s = grid.GetSize();
  float unit = _df->distUnit;
  const unsigned zAxis = 2;
  for (unsigned x = 0; x < s[0] - 1; x++) {
    for (unsigned y = 0; y < s[1] - 1; y++) {
      for (unsigned z = 0; z < s[2] - 1; z++) {
        MarchOneCube(x, y, z, grid, level / unit, surf, _df->voxSize / unit);
      }
    }
  }
  for (unsigned i = 0; i < surf->v.size(); i += 3) {
    surf->v[i] = unit * surf->v[i] + _df->origin[0];
    surf->v[i + 1] = unit * surf->v[i + 1] + _df->origin[1];
    surf->v[i + 2] = unit * surf->v[i + 2] + _df->origin[2];
  }
}

void ComputeCoarseDist(AdapDF* df) {
  df->ClearTemporaryArrays();
  Utils::Stopwatch timer;
  timer.Start();
  df->BuildTrigList(df->mesh_);
  float ms = timer.ElapsedMS();
  timer.Start();
  df->Compress();
  ms = timer.ElapsedMS();
  timer.Start();
  df->mesh_->ComputePseudoNormals();
  ms = timer.ElapsedMS();
  timer.Start();
  df->ComputeCoarseDist();
  ms = timer.ElapsedMS();
  Array3D8u frozen;
  timer.Start();
  df->FastSweepCoarse(frozen);
}
