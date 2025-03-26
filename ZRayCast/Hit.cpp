#include "Hit.h"

#include <algorithm>

int sortHits(HitList& hits, const std::vector<float>* normals) {
  size_t numHits = hits.size();
  if (numHits == 0) {
    return 0;
  }

  std::sort(hits.begin(), hits.end());
  // calculate sign count
  int count = 0;
  float normalZ = 0.0f;
  for (size_t h = 0; h < numHits; h++) {
    unsigned trigId = hits[h].count;
    normalZ = (*normals)[trigId * 3 + 2];
    // ignore normalZ == 0.
    // entering a solid
    if (normalZ < 0) {
      count++;
    } else if (normalZ > 0) {
      count--;
    }
    hits[h].count = count;
  }
  return count;
}
