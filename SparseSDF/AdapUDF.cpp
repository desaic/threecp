#include "AdapUDF.h"
#include "BBox.h"
#include "FastSweep.h"
#include "TrigMesh.h"

AdapUDF::AdapUDF() : AdapDF() {}

void AdapUDF::FastSweepCoarse(Array3D<uint8_t>& frozen) {
  FastSweepParUnsigned(dist, voxSize, distUnit, band, frozen);
}

float AdapUDF::MinDist(const std::vector<size_t>& trigs, const std::vector<TrigFrame>& trigFrames,
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
    float trigz = pv0.dot(frame.z);
    float d = std::sqrt(info.sqrDist + trigz * trigz);
    if (minDist > d) {
      minDist = d;
    }
  }
  return minDist;
}
