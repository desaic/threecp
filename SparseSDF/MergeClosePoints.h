#ifndef MERGE_CLOSE_POINTS_H
#define MERGE_CLOSE_POINTS_H

#include <vector>

#include "Vec3.h"

void MergeClosePoints(const std::vector<Vec3f>& v,
                      std::vector<size_t>& chosenIdx, float eps);

#endif