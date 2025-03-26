#pragma once
#include "Vec3.h"
#include <vector>

struct BBox{
	///vertices at min and max coordinates
	Vec3f vmin, vmax;
	BBox():vmin(0.0f,0.0f,0.0f),
		vmax(0.0f,0.0f,0.0f) {}
  void Merge(const BBox& b);
  bool _init = false;
};

///oriented bounding box
struct OBBox {
  Vec3f origin;
  Vec3f axes[3];
  OBBox() : origin(0) {
    axes[0][0] = 1;
    axes[1][1] = 1;
    axes[2][2] = 1;
  }
};

struct IntBox {
  /// vertices at min and max coordinates
  Vec3i min, max;
  IntBox() : min(0, 0, 0), max(0, 0, 0) {}
};

/// @TODO: phase out BBox and use Box3f
template<typename T>
class BoxTemplate {
public:
  /// vertices at min and max coordinates
  Vec3<T> vmin, vmax;
  bool _init = false;
  BoxTemplate() : vmin(0, 0, 0), vmax(0, 0, 0) {}
  void Merge(const BoxTemplate<T>& b);
};

using Box3i = BoxTemplate<int>;
using Box3u = BoxTemplate<unsigned>;
using Box3f = BoxTemplate<float>;

void UpdateBBox(const std::vector<float>& verts, BBox& box);

void ComputeBBox(const std::vector<float>& verts, BBox& box);
void ComputeBBox(Vec3f v0, Vec3f v1, Vec3f v2, BBox& box);
void ComputeBBox(const std::vector<Vec3f>& verts, BBox& box);