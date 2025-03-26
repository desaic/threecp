#ifndef ADAP_DF_H
#define ADAP_DF_H

#include <memory>

#include "FixedGrid3D.h"
#include "Array3D.h"
#include "PointTrigDist.h"
#include "SDFMesh.h"
#include "Sparse3DMap.h"
#include "SparseNode4.h"

class TrigMesh;

/// <summary>
/// Abstract class Adaptive distance field for AdapSDF and AdapUDF.
/// Stores a dense coarse grid and a sparse set of fine grids.
/// to save space, uses int16_t to store distance values
/// then scaled by 'distUnit' parameter. default 1e-3 unit.
/// </summary>
class AdapDF {
 public:
  AdapDF();
  /// <summary>
  /// inputs are voxel grid size. vertex grid size would be allocated
  /// to be +1 of voxel grid size.
  /// coarse grid will be ceil(voxel grid size/4).
  /// </summary>
  /// <returns>-1 if too many voxels are requested</returns>
  virtual int Allocate(unsigned sx, unsigned sy, unsigned sz);

  unsigned AddDenseCell(const Vec3u& gridIdx);

  void BuildTrigList(TrigMesh* mesh);
  // uses triangle list.
  void ComputeCoarseDist();
  void ComputeCoarseDist(unsigned x, unsigned y, unsigned z);
  /// <summary>
  /// Goes through every fine vertex and compute its distance within voxSize.
  /// Uses the triangle frames and trigList which are computed while ComputeCoarseDist().
  /// </summary>
  void ComputeFineDistBrute();
  virtual void FastSweepCoarse(Array3D<uint8_t>& frozen);
  void FastSweepFine();
  void PropagateNeighbors();
  void ComputeFineCellDist(size_t sparseIdx);
  /// compute point-triangle distances between a fine grid and triangles
  void ComputeFinePtTrig(unsigned x, unsigned y, unsigned z);
  void ComputeDistGrid5(Vec3f x0, FixedGrid3D<5>& fine, const std::vector<size_t>& trigs);
  void GatherTrigs(unsigned x, unsigned y, unsigned z, std::vector<size_t>& trigs);
  bool HasCellDense(const Vec3u& gridIdx) const;
  bool HasCellSparse(const Vec3u& gridIdx) const;

  /// <summary>
  /// Clears trigList and trigFrames.
  /// </summary>
  void ClearTemporaryArrays();

  /// <summary>
  /// After adding points, compress to save memory.
  /// Dense operations no longer valid after this point.
  /// </summary>
  void Compress();

  Vec3f WorldCoord(const Vec3f& gridCoord) const;

  /// <summary>
  /// Biliearly interpolates the vertex distance grid.
  /// </summary>
  /// <returns>distance in mm</returns>
  float GetCoarseDist(const Vec3f& x) const;
  /// <summary>
  /// taken as the min of the bilinear interpolated distance
  /// and the point sample based distance.
  /// </summary>
  /// <returns>distance in mm</returns>
  float GetFineDist(const Vec3f& x) const;

  SparseNode4<unsigned>& GetSparseNode4(unsigned x, unsigned y, unsigned z);
  const SparseNode4<unsigned>& GetSparseNode4(unsigned x, unsigned y, unsigned z) const;
  unsigned GetSparseCellIdx(unsigned x, unsigned y, unsigned z) const;

  virtual float MinDist(const std::vector<size_t>& trigs, const std::vector<TrigFrame>& trigFrames,
                        const Vec3f& query, const TrigMesh* mesh) = 0;
  /// 2 bils.
  static const size_t MAX_NUM_VOX = 1u << 31;

  /// band causes furthur expansion of the grid.
  static const unsigned MAX_BAND = 16;

  // max positive value of short int
  static const short MAX_DIST = 0x7fff;

  // distance values are stored on grid vertices.
  // vertex grid size is voxel grid size +1.
  // stores only 16-bit ints to save memory.
  // distance values won't be too large because of band.
  // physical distance = dist * distUnit.
  Array3D<short> dist;

  TrigMesh* mesh_ = nullptr;
  // coarse grid contains index into list of refined cells.
  //
  // only a sparse subset of voxels have refined cells.
  // sparse nodes are stored at 1/4 resolution of the full grid.
  Sparse3DMap<unsigned> sparseGrid;

  // flat list of fine cells indexed by sparseGrid.
  // index 0 is reserved for empty cells.
  std::vector<FixedGrid3D<5>> fineGrid;
  // temporary lists of triangles intersecting coarse voxels
  // cleared after initialization.
  std::vector<std::vector<size_t>> trigList;
  // temporary additional triangle information
  std::vector<TrigFrame> trigFrames;
  // mm. default is 1um.
  float distUnit = 0.001f;

  Vec3f origin = {0.0f, 0.0f, 0.0f};

  // in mm. square voxels only.
  float voxSize = 0.4;
  // 1/4 of voxSize by default.
  float pointSpacing = 0.1f;
  unsigned band = 5;
};

// implements SDFImp interface
class SDFImpAdap : public SDFImp {
 public:
  SDFImpAdap(std::shared_ptr<AdapDF> df) : _df(df) {}

  float DistNN(Vec3f coord) override;

  /// get distance using triliear interpolation. queries 8 vertices.
  /// coordinate in origininal mesh model space.
  float DistTrilinear(Vec3f coord) override;

  // compute marching cubes using the coarse grid only.
  void MarchingCubes(float level, TrigMesh* surf) override;

  int Compute() override;

  AdapDF* GetSDF() { return _df.get(); }

 private:
  std::shared_ptr<AdapDF> _df;
};
void ComputeCoarseDist(AdapDF* df);
#endif
