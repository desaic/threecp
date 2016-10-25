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

  Eigen::Vector3f v1 = r.o + r.d*tmin;
  Eigen::Vector3f v2 = r.o + r.d*tmax;
  v2 = v2 - v1;
  int point[3];
  for (int i = 0; i < 3; i++) {
    point[i] = (int)(v1[i] / grid.dx[i]);
  }

  int stepX = (v2[0]< 0) ? -1 : 1;
  int stepY = (v2[1]< 0) ? -1 : 1;
  int stepZ = (v2[2] < 0) ? -1 : 1;

  int X = point[0];
  int Y = point[1];
  int Z = point[2];
  grid.clamp(X, Y, Z);
  float tMaxX = ((point[0] + stepX) * grid.dx[0] - v1[0]) / r.d[0];
  float tMaxY = ((point[1] + stepY) * grid.dx[1] - v1[1]) / r.d[1];
  float tMaxZ = ((point[2] + stepZ) * grid.dx[2] - v1[2]) / r.d[2];
  float tDeltaX = std::abs(grid.dx[0] / r.d[0]);
  float tDeltaY = std::abs(grid.dx[1] / r.d[1]);
  float tDeltaZ = std::abs(grid.dx[2] / r.d[2]);
  int val = grid(X, Y, Z);
  if (val >= 0) {
    return val;
  }
  do {
    if (tMaxX < tMaxY) {
      if (tMaxX < tMaxZ) {
        X = X + stepX;
        if (X >= grid.gridSize[0] || X<0)
          break; /* outside grid */
        tMaxX = tMaxX + tDeltaX;
      }
      else {
        Z = Z + stepZ;
        if (Z >= grid.gridSize[2] || Z<0)
          break;
        tMaxZ = tMaxZ + tDeltaZ;
      }
    }
    else {
      if (tMaxY < tMaxZ) {
        Y = Y + stepY;
        if (Y >= grid.gridSize[1] || Y<0)
          break;
        tMaxY = tMaxY + tDeltaY;
      }
      else {
        Z = Z + stepZ;
        if (Z >= grid.gridSize[2] || Z<0)
          break;
        tMaxZ = tMaxZ + tDeltaZ;
      }
    }
    val = grid(X,Y,Z);
  } while (val < 0);
  
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
