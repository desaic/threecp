#pragma once

#include "Array2D.h"
#include "Vec2.h"
#include "Vec3.h"

/// <summary>
/// constant time median filter
/// </summary>
/// <param name="_src"></param>
/// <param name="_dst"></param>
/// <param name="rad"></param>
void medianBlur_8u_O1(const Array2D8u& _src, Array2D8u& _dst, unsigned rad);

/// sorting network median filter for m=3 or 5 only
/// @param m filter window size , not radius. window size = 2*radius + 1.
void medianBlur_SortNet(const Array2D8u& _src, Array2D8u& _dst, int m);

/// wrapper that switches based on filter radius.
/// uses SortNet if rad is 1 or 2.
void MedianBlur(const Array2D8u& src, Array2D8u& dst, unsigned rad);
