#include "Render.hpp"

int rayGridIntersect(const Ray & r0, const RegGrid & grid)
{
  float tmin = 0, tmax = 0;

  //transform to grid coordinates.
  Ray r;
  r.d = r0.d;
  r.o = r0.o - grid.o.cast<float>();
  Eigen::Vector3f bounds[2];
  bounds[0] = Eigen::Vector3f(0, 0, 0);
  for (int i = 0; i < 3; i++) {
    bounds[1][i] = grid.dx[i] * grid.gridSize[i];
  }
  bool boxInter = rayBoxIntersect(r, bounds, tmin, tmax);
  if (!boxInter) {
    return -1;
  }

  // Bresenham3D
  Eigen::Vector3f v1 = r.o + r.d*tmin;
  Eigen::Vector3f v2 = r.o + r.d*tmax;
  int point[3];
  int dx[3];
  for (int i = 0; i < 3; i++) {
    point[i] = (int)(v1[i] / grid.dx[i]);
    dx[i] = (int)(v2[i] / grid.dx[i]) - point[i];
  }

  int i, l, m, n, x_inc, y_inc, z_inc, err_1, err_2, dx2, dy2, dz2;

  x_inc = (dx[0]< 0) ? -1 : 1;
  l = abs(dx[0]);
  y_inc = (dx[1]< 0) ? -1 : 1;
  m = abs(dx[1]);
  z_inc = (dx[2] < 0) ? -1 : 1;
  n = abs(dx[2]);
  dx2 = l << 1;
  dy2 = m << 1;
  dz2 = n << 1;

  if ((l >= m) && (l >= n)) {
    err_1 = dy2 - l;
    err_2 = dz2 - l;
    for (i = 0; i < l; i++) {
      int val = grid(point[0], point[1], point[2]);
      if (val >= 0) {
        return val;
      }
      if (err_1 > 0) {
        point[1] += y_inc;
        err_1 -= dx2;
      }
      if (err_2 > 0) {
        point[2] += z_inc;
        err_2 -= dx2;
      }
      err_1 += dy2;
      err_2 += dz2;
      point[0] += x_inc;
    }
  }
  else if ((m >= l) && (m >= n)) {
    err_1 = dx2 - m;
    err_2 = dz2 - m;
    for (i = 0; i < m; i++) {
      int val = grid(point[0], point[1], point[2]);
      if (val >= 0) {
        return val;
      }
      if (err_1 > 0) {
        point[0] += x_inc;
        err_1 -= dy2;
      }
      if (err_2 > 0) {
        point[2] += z_inc;
        err_2 -= dy2;
      }
      err_1 += dx2;
      err_2 += dz2;
      point[1] += y_inc;
    }
  }
  else {
    err_1 = dy2 - n;
    err_2 = dx2 - n;
    for (i = 0; i < n; i++) {
      int val = grid(point[0], point[1], point[2]);
      if (val >= 0) {
        return val;
      }
      if (err_1 > 0) {
        point[1] += y_inc;
        err_1 -= dz2;
      }
      if (err_2 > 0) {
        point[0] += x_inc;
        err_2 -= dz2;
      }
      err_1 += dy2;
      err_2 += dx2;
      point[2] += z_inc;
    }
  }
  int val = grid(point[0], point[1], point[2]);
  return val;

}

bool rayBoxIntersect(const Ray & r, const Eigen::Vector3f * bounds,
  float & tmin, float & tmax)
{
  int sign[3];
  Eigen::Vector3f invdir(1.0 / r.d[0], 1.0 / r.d[1], 1.0 / r.d[2]);
  sign[0] = (invdir[0] < 0);
  sign[1] = (invdir[1] < 0);
  sign[2] = (invdir[2] < 0);

  float tymin, tymax, tzmin, tzmax;

  tmin = (bounds[sign[0]][0] - r.o[0]) * invdir[0];
  tmax = (bounds[1 - sign[0]][0] - r.o[0]) * invdir[0];
  tymin = (bounds[sign[1]][1] - r.o[1]) * invdir[1];
  tymax = (bounds[1 - sign[1]][1] - r.o[1]) * invdir[1];

  if ((tmin > tymax) || (tymin > tmax))
    return false;
  if (tymin > tmin)
    tmin = tymin;
  if (tymax < tmax)
    tmax = tymax;

  tzmin = (bounds[sign[2]][2] - r.o[2]) * invdir[2];
  tzmax = (bounds[1 - sign[2]][2] - r.o[2]) * invdir[2];

  if ((tmin > tzmax) || (tzmin > tmax))
    return false;
  if (tzmin > tmin)
    tmin = tzmin;
  if (tzmax < tmax)
    tmax = tzmax;

  return true;
}
