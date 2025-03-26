#pragma once
///\file Hit.h
/// struct for ray intersections
#include <array>
#include <vector>

#include "Vec2.h"

/// ray intersection
struct Hit {
  Hit() : t(-1), count(-1) {}

  Hit(float t, int pID = -1) : t(t), count(pID) {}

  friend bool operator<(const Hit& a, const Hit& b) {
    if (a.t == b.t) {
      return a.count < b.count;
    }
    return a.t < b.t;
  }

 public:
  /// distance of hit from ray origin.
  float t;
  /// ray intersection count.
  /// starts at 0. +1 for each entrance and -1 for each exit.
  int count;
};

typedef std::vector<Hit> HitList;

using HitList4 = std::array<HitList, 4>;

///\return 0 if the hits have valid signs
/// otherwise returns the total normal count
int sortHits(HitList& hits, const std::vector<float>* normals);
