#pragma once

#include "Vec2.h"
#include "Vec3.h"

enum class TrigPointType { V0, V1, V2, E01, E12, E02, FACE };

struct TrigDistInfo {
  /// squared distance
  float sqrDist;
  Vec3f bary;
  TrigPointType type;
};

struct TrigFrame {
  // triangle frame
  Vec3f x, y, z;

  // transformed triangle.
  float v1x = 0;
  float v2x = 0;
  float v2y = 0;
};

// v0 v1 is x axis. v0 is origin.
// v1y = 0.
// normal is z
void ComputeTrigFrame(const float* trig, const Vec3f& n, TrigFrame & frame);

TrigDistInfo PointTrigDist2D(float px, float py, float t1x, float t2x,
                             float t2y);
