#include "ABuffer.h"
/// converts hits to a list of line segments to store in alpha buffer.
/// merges overlapping segments or segments within zRes to
///  a single segment.
int ConvertHitsToSegs(const HitList& hits, ABufferf::SegList& segs, float zRes) {
  if (hits.size() == 0) {
    return 0;
  }
  const float zEps = 5e-3;
  // initial exit won't be merged to segment.
  float prevExit = hits[0].t - 2 * zEps;
  // starting from outside.
  bool inside = false;
  ABufferf::Segment seg;
  std::vector<ABufferf::Segment> segvec;
  for (size_t i = 0; i < hits.size(); i++) {
    if (hits[i].t > MAX_Z_HEIGHT_MM) {
      return -2;
    }
    if (inside) {
      // look for exit
      bool hasExit = false;
      // same loop variable i
      for (; i < hits.size(); i++) {
        bool outside = (hits[i].count == 0);
        if (outside) {
          hasExit = true;
          break;
        }
      }

      if (!hasExit) {
        // reached end of hit list without an exit.
        // discard the last segment.
        break;
      }
      seg.t1 = hits[i].t;
      inside = false;
      if (segs.size() > 0 && (seg.t0 - prevExit < zEps)) {
        // merge close segments
        segvec.back().t1 = seg.t1;
      } else {
        segvec.push_back(seg);
      }
      prevExit = seg.t1;
    } else {
      // look for entrance
      bool hasEntrance = false;
      // same i variable as the outer loop.
      for (; i < hits.size(); i++) {
        bool in = (hits[i].count > 0);
        if (in) {
          hasEntrance = true;
          break;
        }
      }
      if (!hasEntrance) {
        // no more points entering the geometry.
        break;
      }
      seg.t0 = hits[i].t;
      inside = true;
    }
  }
  segs.Allocate(segvec.size());
  for (size_t i = 0; i < segvec.size(); i++) {
    segs[i] = segvec[i];
  }
  return 0;
}