#pragma once

#include <vector>

namespace MeshOptimization {
class Vec3fKey {
  public:
  Vec3fKey() {}
  Vec3fKey(float x, float y, float z):x(x), y(y), z(z) {}
  float eps = 1e-3;
  float x = 0;
  float y = 0;
  float z = 0;
  bool operator==(const Vec3fKey& other) const {
    return std::abs(x - other.x) <= eps &&
            std::abs(y - other.y) <= eps &&
            std::abs(z - other.z) <= eps;
  }
};

struct Vec3fHash {
  size_t operator()(const Vec3fKey& k) const {
    const float eps = 1e-3;
    return size_t((31 * 17) * int64_t(k.x / eps) +
                  31 * int64_t(k.y / eps) + int64_t(k.z / eps));
  }
};

void ComputeSimplifiedMesh(std::vector<float>& V_in, std::vector<uint32_t>& T_in, 
                           float targetError, float targetCountFraction, 
                           std::vector<float>& V_out, std::vector<uint32_t>& T_out);
void MergeCloseVertices(std::vector<float>& V_in, std::vector<uint32_t>& T_in, 
                       std::vector<float>& V_out);
void RemoveInternalTriangles(const std::vector<float>& vertices_in,
                             const std::vector<uint32_t>& triangles_in,
                             std::vector<float>& vertices_out, std::vector<uint32_t>& triangles_out,
                             float voxRes = 1.0f);
}
  