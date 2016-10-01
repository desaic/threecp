#include <Eigen\Dense>
#include "linmath.h"

Eigen::Matrix4f mat4x4_perspective(float y_fov, float aspect, float n, float f)
{
  Eigen::Matrix4f m = Eigen::Matrix4f::Zero();
  float const a = 1.f / (float)tan(y_fov / 2.f);

  m(0, 0) = a / aspect;
  m(1, 1) = a;

  m(2, 2) = -((f + n) / (f - n));
  m(2, 3) = -1.f;

  m(3, 2) = -((2.f * f * n) / (f - n));
  m.transposeInPlace();
  return m;
}

Eigen::Matrix4f mat4x4_look_at(Eigen::Vector3f eye, Eigen::Vector3f center, Eigen::Vector3f up)
{
  Eigen::Vector3f f;
  Eigen::Vector3f  s;
  Eigen::Vector3f  t;

  Eigen::Matrix4f m = Eigen::Matrix4f::Zero();
  f = center - eye;
  f.normalize();

  s = f.cross(up);
  s.normalize();

  t=s.cross(f);

  m(0,0) = s[0];
  m(0,1) = t[0];
  m(0,2) = -f[0];

  m(1,0) = s[1];
  m(1,1) = t[1];
  m(1,2) = -f[1];

  m(2,0) = s[2];
  m(2,1) = t[2];
  m(2,2) = -f[2];

  m(3,3) = 1.f;

  mat4x4_translate_in_place(m, -eye[0], -eye[1], -eye[2]);
  m.transposeInPlace();
  return m;
}

void mat4x4_translate_in_place(Eigen::Matrix4f & M, float x, float y, float z)
{
  Eigen::Vector4f t;
  t << x, y, z, 0 ;
  Eigen::Vector4f r;
  int i;
  for (i = 0; i < 4; ++i) {
    r = M.row(i);
    M(3,i) += r.transpose() * t;
  }
}
