#include "AdapSDF.h"
#include "BBox.h"
#include "FastSweep.h"
#include "TrigMesh.h"

AdapSDF::AdapSDF() : AdapDF() {}

float AdapSDF::MinDist(const std::vector<size_t>& trigs, const std::vector<TrigFrame>& trigFrames,
                       const Vec3f& query, const TrigMesh* mesh) {
  float minDist = 1e20;
  for (size_t i = 0; i < trigs.size(); i++) {
    float px, py;
    size_t tIdx = trigs[i];
    const TrigFrame& frame = trigFrames[tIdx];
    Triangle trig = mesh->GetTriangleVerts(tIdx);
    Vec3f pv0 = query - trig.v[0];
    px = pv0.dot(frame.x);
    py = pv0.dot(frame.y);
    TrigDistInfo info = PointTrigDist2D(px, py, frame.v1x, frame.v2x, frame.v2y);

    Vec3f normal = mesh->GetNormal(tIdx, info.bary);
    Vec3f closest = info.bary[0] * trig.v[0] + info.bary[1] * trig.v[1] + info.bary[2] * trig.v[2];

    float trigz = pv0.dot(frame.z);
    // vector from closest point to voxel point.
    Vec3f trigPt = query - closest;
    float d = std::sqrt(info.sqrDist + trigz * trigz);
    if (std::abs(minDist) > d) {
      if (normal.dot(trigPt) < 0) {
        minDist = -d;
      } else {
        minDist = d;
      }
    }
  }
  return minDist;
}
