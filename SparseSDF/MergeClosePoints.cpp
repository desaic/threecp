#include "MergeClosePoints.h"
#include <unordered_map>

class Vec3fKey {
 public:
  Vec3fKey() {}
  Vec3fKey(float x, float y, float z, float e):v(x,y,z),eps(e) {}
  Vec3f v;
  float eps = 1e-3;
  bool operator==(const Vec3fKey& other) const {
    return std::abs(v[0] - other.v[0]) <= eps &&
           std::abs(v[1] - other.v[1]) <= eps &&
           std::abs(v[2] - other.v[2]) <= eps;
  }
};

struct Vec3fHash {
  size_t operator()(const Vec3fKey& k) const {
    float eps = k.eps;
    return size_t((31 * 17) * int64_t(k.v[0] / eps) +
                  31 * int64_t(k.v[1] / eps) + int64_t(k.v[2] / eps));
  }
};

void MergeClosePoints(const std::vector<Vec3f>& v,
                      std::vector<size_t>& chosenIdx, float eps) {
  /// maps from a vertex to its new index.
  std::unordered_map<Vec3fKey, size_t, Vec3fHash> vertMap;
  size_t vCount = 0;
  chosenIdx.clear();
  for (size_t i = 0; i < v.size(); i++) {
    Vec3fKey vi(v[i][0], v[i][1], v[i][2], eps);
    auto it = vertMap.find(vi);
    if (it == vertMap.end()) {
      vertMap[vi] = vCount;
      chosenIdx.push_back(i);
      vCount++;
    }
  }
}