#ifndef CAMERA_H
#define CAMERA_H

#include "Vec3.h"
#include "Matrix4f.h"

struct Camera {
  Camera();

  void rotate(float dx, float dy);
  void pan(float dx, float dy);
  void zoom(float ratio);
  void update();

  Vec3f eye;
  Vec3f at;
  Vec3f up;
  float near=1, far=100.0f;
  float fovRad = 0.7;
  
  // view times projection
  // @param dst 16 numbers, column major because glsl.
  void VP(float * dst);
  // inverse transpose of view matrix
  // does nothing to rotation
  void VIT(float* dst);

  Matrix4f view, proj;

  // aspect ratio for perspective matrix;
  float aspect = 1.0f;
};
#endif