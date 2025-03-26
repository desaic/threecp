#include "RasterizeTrig.h"

void UpdateInterval(Vec2i& interval, int x) {
  if (interval[0] < 0) {
    interval[0] = x;
    interval[1] = x;
  } else {
    if (x > interval[1]) {
      interval[1] = x;
    } else if (x < interval[0]) {
      interval[0] = x;
    }
  }
}

void RasterizeLine(Vec2f a, Vec2f b, std::vector<Vec2i>& intervals) {
  Vec2f dir = b - a;
  int row_a = int(a[1]);
  int row_b = int(b[1]);
  int maxRow = std::max(row_a, row_b);
  if (int(intervals.size()) < maxRow + 1) {
    return;
  }
  if (row_a == row_b) {
    // horizontal line.
    int col_a = int(a[0]);
    int col_b = int(b[0]);
    if (col_a > col_b) {
      int tmp = col_a;
      col_a = col_b;
      col_b = tmp;
    }
    if (intervals[row_a][0] < 0) {
      intervals[row_a][0] = col_a;
      intervals[row_a][1] = col_b;
    } else {
      intervals[row_a][0] = std::min(intervals[row_a][0], col_a);
      intervals[row_a][1] = std::max(intervals[row_a][1], col_b);
    }
    return;
  }

  if (dir[1] < 0) {
    Vec2f tmp = a;
    a = b;
    b = tmp;
    dir = -dir;
  }

  float t = 0;
  int ix = int(a[0]);
  int iy = int(a[1]);
  int delta_ix = dir[0] > 0 ? 1 : -1;
  float epsilon = 1e-6f;
  float dx = std::abs(dir[0]);
  while (t < 1) {
    float distx = 0;
    Vec2f point = a + t * dir;
    if (dir[0] > 0) {
      distx = (ix + 1) - point[0];
    } else {
      distx = ix - point[0];
    }
    float abs_distx = (distx >= 0 ? distx : (-distx));
    if (intervals[iy][0] < 0) {
      // initialize to 1-pixel wide interval
      intervals[iy][0] = ix;
      intervals[iy][1] = ix;
    } else {
      UpdateInterval(intervals[iy], ix);
    }
    float disty = (iy + 1) - point[1];
    float next_t = 0;

    // distx/dir[0] vs disty/dir[1]
    // but avoid division
    if (abs_distx * dir[1] < disty * dx) {
      // hit vertical wall first
      // now division is safe.
      next_t = abs_distx / dx;
      ix += delta_ix;
    } else {
      next_t = disty / dir[1];
      iy++;
    }
    t = t + next_t + epsilon;
  }
  ix = int(b[0]);
  iy = int(b[1]);
  if (intervals[iy][0] < 0) {
    intervals[iy][0] = ix;
    intervals[iy][1] = ix;
  } else {
    UpdateInterval(intervals[iy], ix);
  }
}

void RasterizeTrig(const Vec2f* trig, std::vector<Vec2i>& row_intervals) {
  RasterizeLine(trig[0], trig[1], row_intervals);
  RasterizeLine(trig[1], trig[2], row_intervals);
  RasterizeLine(trig[2], trig[0], row_intervals);
}
