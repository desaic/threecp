#include "MeshOptimization.h"
#include "meshoptimizer.h"
#include <array>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <algorithm>
#include <execution>
#include <numeric>
#include <queue>
#include "voxelize_shell.h"
#include "Array3D.h"
#include "BBox.h"

namespace MeshOptimization {
void MergeCloseVertices(std::vector<float>& v, std::vector<uint32_t>& t, std::vector<float>& v_out) {
  /// maps from a vertex to its new index.
  std::vector<float> v_tmp;

  std::unordered_map<Vec3fKey, size_t, Vec3fHash> vertMap;
  size_t nV = v.size() / 3;
  std::vector<uint32_t> newVertIdx(nV, 0);  
  size_t vCount = 0;

  for (size_t i = 0; i < v.size(); i+=3) {
    Vec3fKey vi(v[i], v[i + 1], v[i + 2]);
    auto it = vertMap.find(vi);
    size_t newIdx = 0;
    if (it == vertMap.end()) {
      vertMap[vi] = vCount;
      newIdx = vCount;
      v_tmp.push_back(v[i]);
      v_tmp.push_back(v[i+1]);
      v_tmp.push_back(v[i+2]);
      vCount++;
    } else {
      newIdx = it->second;
    }
    newVertIdx[i / 3] = newIdx;
  }
  std::vector<uint32_t> t_out;
  for (size_t i = 0; i < t.size(); i += 3) {
    unsigned t0 = newVertIdx[t[i]];
    unsigned t1 = newVertIdx[t[i + 1]];
    unsigned t2 = newVertIdx[t[i + 2]];
    if (t0 == t1 || t0 == t2 || t1 == t2) {
      // triangle with duplicated vertices.
      continue;
    }
    t_out.push_back(t0);
    t_out.push_back(t1);
    t_out.push_back(t2);
  }
  v_out = std::move(v_tmp);
  t = std::move(t_out);
}

void SimplifiedMeshParallel(std::vector<float>& V_in, std::vector<uint32_t>& T_in,
  float targetError, float targetCountFraction,
  std::vector<float>& V_out, std::vector<uint32_t>& T_out) {

  float result_error = 0;
  int nVertsIn = V_in.size() / 3;
  int nFacesIn = T_in.size() / 3;
  
  // splits vertex based on octant
  float xbar = 0, ybar = 0, zbar = 0;
  for (int i = 0; i < V_in.size(); i += 3) {
    xbar += V_in[i];
    ybar += V_in[i + 1];
    zbar += V_in[i + 2];
  }
  xbar /= nVertsIn;
  ybar /= nVertsIn;
  zbar /= nVertsIn;
  auto octantOf = [&](float x, float y, float z) {
    return ((x > xbar) << 2) | ((y > ybar) << 1) | (z > zbar);
  };
  std::vector<std::vector<float>> V_in_split(8);

  std::vector<std::vector<uint32_t>> remap_tooldidx(8); // map from [octant , new index] to old index
  std::vector<uint32_t> remap_miniindex; // map from old index to new index
  std::vector<uint32_t> remap_octant; // map from old index to octant
  for (int i = 0; i < V_in.size(); i += 3) {
    auto x = V_in[i];
    auto y = V_in[i + 1];
    auto z = V_in[i + 2];
    auto octant = octantOf(x, y, z);
    remap_tooldidx[octant].push_back(i / 3);
    remap_miniindex.push_back(V_in_split[octant].size() / 3);
    remap_octant.push_back(octant);
    V_in_split[octant].push_back(x);
    V_in_split[octant].push_back(y);
    V_in_split[octant].push_back(z);
  }
  std::vector<size_t> V_in_split_offset;
  V_in_split_offset.push_back(0);
  for (int i = 1; i < 8; i++) {
    V_in_split_offset.push_back(V_in_split_offset[i - 1] + V_in_split[i - 1].size());
  }

  // splits the index based on octant also (face on the same octant got into each octant)
  // face that goes across octant gets directly placed in the merged T out directly
  std::vector<std::vector<uint32_t>> T_in_split(8);
  std::vector<std::vector<uint32_t>> T_out_split(8);
  std::vector<uint32_t> merged_T_out;

  for (int i = 0; i < T_in.size(); i+=3) {
    auto idx1 = T_in[i];
    auto idx2 = T_in[i + 1];
    auto idx3 = T_in[i + 2];
    auto octant1 = remap_octant[idx1];
    auto octant2 = remap_octant[idx2];
    auto octant3 = remap_octant[idx3];
    if (octant1 == octant2 && octant2 == octant3) {
      auto miniidx1 = remap_miniindex[idx1];
      auto miniidx2 = remap_miniindex[idx2];
      auto miniidx3 = remap_miniindex[idx3];
      T_in_split[octant1].push_back(miniidx1);
      T_in_split[octant1].push_back(miniidx2);
      T_in_split[octant1].push_back(miniidx3);
    }
    else {
      merged_T_out.push_back(idx1);
      merged_T_out.push_back(idx2);
      merged_T_out.push_back(idx3);
    }
  }
 
  // parallel simplify
  std::vector<int> blockIdx(8);
  for (int i = 0; i < 8; i++) {
    T_out_split[i].resize(T_in_split[i].size());
  }
  std::iota(blockIdx.begin(), blockIdx.end(), 0);
  std::for_each(std::execution::par_unseq, blockIdx.begin(), blockIdx.end(), [&](int block_idx) {
    auto& cur_V_in = V_in_split[block_idx];
    auto& cur_T_in = T_in_split[block_idx];
    auto& cur_T_out = T_out_split[block_idx];
    int nVert = cur_V_in.size() / 3;
    if (nVert == 0 || cur_T_in.size() == 0) return;
    size_t targetIndexCountMini = std::max(1ull, size_t(cur_T_in.size() * targetCountFraction));
    cur_T_out.resize(meshopt_simplify(&cur_T_out[0], &cur_T_in[0], cur_T_in.size(), &cur_V_in[0],
                                      nVert, 3 * sizeof(float), targetIndexCountMini, targetError,
                                      meshopt_SimplifyLockBorder, &result_error));
  });

  for (int i = 0; i < 8; i ++) {
    for (auto t : T_out_split[i]) {
      merged_T_out.push_back(remap_tooldidx[i][t]);
    }
  }

  size_t targetIndexCount = std::max(1ull, size_t(merged_T_out.size() * targetCountFraction));

  T_out.resize(merged_T_out.size());
  T_out.resize(meshopt_simplify(&T_out[0], &merged_T_out[0], merged_T_out.size(), &V_in[0], nVertsIn, 3 * sizeof(float), targetIndexCount, targetError, 0, &result_error));

  
}


void ComputeSimplifiedMesh(std::vector<float>& V_in, std::vector<uint32_t>& T_in, 
                           float targetErrorAbs, float targetCountFraction,
                           std::vector<float>& V_out, std::vector<uint32_t>& T_out)
{

  size_t targetIndexCount = std::max(1ull, size_t(T_in.size() * targetCountFraction));
  // compute targetError from mm to relative
  BBox box;
  if (V_in.size() == 0) {
    return;
  }
  ComputeBBox(V_in, box);
  Vec3f len = box.vmax - box.vmin;
  float maxLen = std::max({len[0], len[1], len[2]});
  // avoid div 0
  float eps = 1e-6;
  float targetErrorRel = targetErrorAbs / (maxLen + eps);

  float result_error = 0;
  int nVertsIn = V_in.size() / 3;
  int nFacesIn = T_in.size() / 3;
  if (nFacesIn > 1e6) {
    SimplifiedMeshParallel(V_in, T_in, targetErrorRel, targetCountFraction, V_out, T_out);
  } else {
    // single thread mesh opt simplify
    T_out.resize(T_in.size());  // note: simplify needs space for index_count elements in the
                                // destination array, not target_index_count
    T_out.resize(meshopt_simplify(&T_out[0], &T_in[0], T_in.size(), &V_in[0], nVertsIn,
                                  3 * sizeof(float), targetIndexCount, targetErrorRel, 0,
                                  &result_error));
  }

  V_out.resize(V_in.size()); // note: this is just to reduce the cost of resize()
  V_out.resize(3 * meshopt_optimizeVertexFetch(&V_out[0], &T_out[0], T_out.size(), &V_in[0], V_in.size(), 3 * sizeof(float)));
  
}

inline Vec3u linearToGridIdx(size_t l, const Vec3u& gridSize) {
  unsigned x = l % gridSize[0];
  unsigned y = (l%(gridSize[0]*gridSize[1]))/gridSize[0];
  unsigned z = l / (gridSize[0] * gridSize[1]);
  return Vec3u(x,y, z);
}

bool InBound(int x, int y, int z, const Vec3u& size) {
  return x >= 0 && y >= 0 && z >= 0 && x < int(size[0]) && y < int(size[1]) && z < int(size[2]);
}

inline uint64_t Array3DLinearIdx(unsigned ix, unsigned iy, unsigned iz, const Vec3u& gridSize) {
  return ix + iy * (uint64_t)gridSize[0] + iz * uint64_t(gridSize[1]) * gridSize[0];
}

enum class VoxLabel : unsigned char { UNVISITED = 0, INSIDE, OUTSIDE, BOUNDARY };

void FindBoundaryVoxels(Array3D<VoxLabel>& grid) {
  Vec3u size = grid.GetSize();
  if (size[0] < 2) {
    return;
  }

  std::queue<size_t> voxq;
  for (unsigned z = 0; z < size[2]; z++) {
    for (unsigned y = 0; y < size[1]; y++) {
      grid(0, y, z) = VoxLabel::OUTSIDE;
      voxq.push(Array3DLinearIdx(1, y, z, size));
    }
  }
  const unsigned NUM_NBR = 6;
  const int nbrOffset[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
  while (!voxq.empty()) {
    size_t linearIdx = voxq.front();
    voxq.pop();
    VoxLabel label = grid.GetData()[linearIdx];
    Vec3u idx = linearToGridIdx(linearIdx, size);
    if (label == VoxLabel::INSIDE || label == VoxLabel::BOUNDARY) {
      grid(idx[0], idx[1], idx[2]) = VoxLabel::BOUNDARY;
      //include one more layer as boundary
      for (unsigned ni = 0; ni < NUM_NBR; ni++) {
        int nx = int(idx[0] + nbrOffset[ni][0]);
        int ny = int(idx[1] + nbrOffset[ni][1]);
        int nz = int(idx[2] + nbrOffset[ni][2]);
        if (!InBound(nx, ny, nz, size)) {
          continue;
        }
        if (grid(unsigned(nx), unsigned(ny), unsigned(nz)) == VoxLabel::INSIDE) {
          grid(unsigned(nx), unsigned(ny), unsigned(nz)) = VoxLabel::BOUNDARY;
        }
      }
      continue;
    }
    if (label == VoxLabel::UNVISITED || label == VoxLabel::OUTSIDE) {
      grid(idx[0], idx[1], idx[2]) = VoxLabel::OUTSIDE;
      for (unsigned ni = 0; ni < NUM_NBR;ni++) {
        int nx = int(idx[0] + nbrOffset[ni][0]);
        int ny = int(idx[1] + nbrOffset[ni][1]);
        int nz = int(idx[2] + nbrOffset[ni][2]);
        if (!InBound(nx, ny, nz, size)) {
          continue;
        }
        unsigned ux = unsigned(nx), uy = unsigned(ny), uz=unsigned(nz);
        VoxLabel nVal = grid(ux, uy, uz);
        size_t nLinear = Array3DLinearIdx(ux, uy, uz, size);
        if (nVal == VoxLabel::UNVISITED) {
          grid(ux, uy, uz) = VoxLabel::OUTSIDE;
          voxq.push(nLinear);
        } else if (nVal == VoxLabel::INSIDE) {
          grid(ux, uy, uz) = VoxLabel::BOUNDARY;
          voxq.push(nLinear);
        }
      }
    }
  }
}

void RemoveInternalTriangles(const std::vector<float>& vertices_in,
                             const std::vector<uint32_t>& triangles_in,
                             std::vector<float>& vertices_out, std::vector<uint32_t>& triangles_out,
                             float voxRes) {

  voxconf conf;
  conf.unit = Vec3f(voxRes, voxRes, voxRes);
  BBox box;
  ComputeBBox(vertices_in, box);
  // add margin for flood filling outside
  Vec3f margin = 1.5f * conf.unit;
  box.vmin = box.vmin - margin;
  box.vmax = box.vmax + margin;
  conf.origin = box.vmin;

  Vec3f count = (box.vmax - box.vmin) / conf.unit;

  conf.gridSize = Vec3u(count[0] + unsigned(2 * margin[0]), count[1] + unsigned(2 * margin[1]),
                        count[2] + unsigned(2 * margin[2]));

  Array3D<VoxLabel> voxGrid;

  voxGrid.Allocate(conf.gridSize[0], conf.gridSize[1], conf.gridSize[2]);
  voxGrid.Fill(VoxLabel::UNVISITED);
  // get a simple binary voxel grid
  voxelize_shell(conf, vertices_in, triangles_in,
                 [&](unsigned x, unsigned y, unsigned z, size_t tidx) {
                   voxGrid(x, y, z) = VoxLabel::INSIDE;
                 });
  FindBoundaryVoxels(voxGrid);
  size_t numTrigIn = triangles_in.size() / 3;
  std::vector<bool> keepTrig(numTrigIn, false);
  voxelize_shell(conf, vertices_in, triangles_in,
                 [&](unsigned x, unsigned y, unsigned z, size_t tidx) {
                   if (voxGrid(x, y, z) == VoxLabel::BOUNDARY) {
                     keepTrig[tidx] = true;
                   }
                 });
  unsigned numVertsNew = 0;
  size_t numVertsIn = vertices_in.size() / 3;
  unsigned InvalidVert = unsigned(numVertsIn);
  //map from old to new vertex index
  std::vector<unsigned> newVertIdx(numVertsIn, InvalidVert);
  vertices_out.clear();
  triangles_out.clear();
  for (size_t t = 0; t < numTrigIn; t++) {
    if (!keepTrig[t]) {
      continue;
    }
    for (unsigned j = 0; j < 3; j++) {
      unsigned vIdx = triangles_in[3 * t + j];
      unsigned newV = newVertIdx[vIdx];
      if (newV == InvalidVert) {
        newV = numVertsNew;
        newVertIdx[vIdx] = newV;
        numVertsNew++;
        for (unsigned k = 0; k < 3; k++) {
          vertices_out.push_back(vertices_in[3*vIdx+k]);
        }
      }
      triangles_out.push_back(newV);
    }
  }
}

}
