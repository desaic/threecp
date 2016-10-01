#include <Eigen\Dense>
#include "linmath.h"

Eigen::Matrix4f mat4x4_perspective(float y_fov, float aspect, float n, float f)
{
  Eigen::Matrix4f m = Eigen::Matrix4f::Zero();
  float const a = 1.f / (float)tan(y_fov / 2.f);

  m(0, 0) = a / aspect;
  m(1, 1) = a;

  m(2, 2) = -((f + n) / (n - f));
  m(3, 2) = 1.f;

  m(2, 3) = ((2.f * f * n) / (n - f));
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

  s = up.cross(f);
  s.normalize();

  t=f.cross(s);
  m.block(0, 0, 1, 3) = s.transpose();
  m.block(1, 0, 1, 3) = t.transpose();
  m.block(2, 0, 1, 3) = f.transpose();
  m(3,3) = 1.f;
  mat4x4_translate_in_place(m, -eye[0], -eye[1], -eye[2]);
  return m;
}

void mat4x4_translate_in_place(Eigen::Matrix4f & M, float x, float y, float z)
{
  Eigen::Vector4f t;
  t << x, y, z, 0 ;
  Eigen::Vector4f r;
  r = M*t;
  M.block(0, 3, 4, 1) += r;
}
