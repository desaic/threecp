#ifndef RASTERIZE_TRIG_H
#define RASTERIZE_TRIG_H
#pragma once

#include <vector>

#include "Vec2.h"

void RasterizeTrig(const Vec2f* trig, std::vector<Vec2i>& row_intervals);

#endif
