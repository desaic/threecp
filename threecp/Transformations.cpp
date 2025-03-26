#include "Transformations.h"
#ifndef M_PI
#define M_PI 3.14159265
#endif 
float degToRadian(float deg) {
  return deg / 180.0f * M_PI;
}

void transformVerts(const std::vector<float> & verts, std::vector<float> & dst, 
  const Matrix3f mat) {
  size_t nV = verts.size() / 3;
  dst.resize(verts.size());
  for (size_t i = 0; i < nV; i++) {
    Vec3f v0(verts[3 * i], verts[3 * i + 1], verts[3 * i + 2]);
    Vec3f v1 = mat * v0;
    dst[3 * i] = v1[0];
    dst[3 * i + 1] = v1[1];
    dst[3 * i + 2] = v1[2];
  }
}

///rotation is applied in order of xyz, the rotation matrix is written as
///M = MzMyMx with Mx multiplied first.
void rotateVerts(const std::vector<float>& verts, std::vector<float>& rotated,
  float rx, float ry, float rz) {

  //directly copy vertices to dest if no rotation needed
  float eps = 1e-4f;
  if (std::abs(rx) < eps && std::abs(ry) < ry && std::abs(rz) < eps) {
    rotated = verts;
    return;
  }

  Matrix3f mat = RotationMatrix(rx,ry,rz);
  transformVerts(verts, rotated, mat);
}

Matrix3f RotationMatrix(float rx, float ry, float rz)
{
  Matrix3f mat;
  mat = Matrix3f::rotateX(degToRadian(rx));
  mat = mat * Matrix3f::rotateY(degToRadian(ry));
  mat = mat * Matrix3f::rotateZ(degToRadian(rz));
  return mat;
}
