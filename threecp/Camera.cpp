#include "Camera.h"

//debug
#include <iostream>

// mat4x4
#include "Matrix4f.h"

Camera::Camera() {
  for (int ii = 0; ii < 3; ii++) {
    eye[ii] = 0.0f;
    at[ii] = 0.0f;
    up[ii] = 0.0f;
  }
  at[2] = -1.0f;
  at[1] = 0.5f;
  eye[2] = -2.0f;
  eye[1] = 0.5f;
  up[1] = 1.0f;
  update();
}

void Camera::rotate(float dx, float dy) { 
  Vec3f viewDir = at - eye;
  float len = viewDir.norm2();
  if (len == 0) {
    return;
  }
  len = std::sqrt(len);
  viewDir = (1.0f / len) * viewDir;
  Vec3f y = Vec3f(0, 1, 0);
  float longitude = 0, latitude = 0;
  if (std::abs(viewDir[1]) > 0.9999f) {    
    longitude = std::atan2(up[2], up[0]);
    if (viewDir[1] > 0) {
      longitude += M_PI;
    }
  } else {
    longitude = std::atan2(viewDir[2], viewDir[0]);
  }
  float xzLen = std::sqrt(viewDir[0] * viewDir[0] + viewDir[2] * viewDir[2]);
  latitude = std::atan2(viewDir[1], xzLen);

  longitude += dx;
  latitude -= dy;
  if (latitude > 0.4999f * M_PI) {
    latitude = 0.4999f * M_PI;
  }
  if (latitude < -0.4999f * M_PI) {
    latitude = -0.4999f * M_PI;
  }
  float c = std::cos(latitude);
  float s = std::sin(latitude);
  viewDir = Vec3f(c* std::cos(longitude), s,c*std::sin(longitude));
 
  up = Vec3f(-s * std::cos(longitude), c, -s * std::sin(longitude));
  eye = at - len * viewDir;
  update();
}

void Camera::pan(float dx, float dy) {
  Vec3f viewDir = at - eye;
  float len = viewDir.norm2();
  if (len == 0) {
    eye = at + Vec3f(0, 0, near);
    viewDir = at - eye;
    len = viewDir.norm2();
  }
  len = std::sqrt(len);
  viewDir = (1.0f / len) * viewDir;

  Vec3f right;
  if (std::abs(viewDir[1]) > 0.999f) {
    right = viewDir.cross(up);
  } else {
    right = viewDir.cross(Vec3f(0, 1, 0));
  }
  right.normalize();
  Vec3f displacement = -dx * right + dy * up;
  at = at + displacement;
  eye = eye + displacement;
  update();
}

void Camera::zoom(float dist) {
  Vec3f viewDir = at - eye;
  float len = viewDir.norm2();
  if (len == 0) {
    eye = at + Vec3f(0, 0, near);
    viewDir = at - eye;
    len = viewDir.norm2();
  }
  len = std::sqrt(len);
  viewDir = (1.0f / len) * viewDir;
  len = len - dist;
  if (len < near) {
    len = near;
    at = at + dist * viewDir;
  }
  if (len > far) {
    len = far;
    at = at + dist * viewDir;
  }
  eye = at - len * viewDir;
  update();
}

void Camera::update() {
  Vec3f viewDir = at-eye;
  viewDir.normalize();
  if (viewDir.norm2() == 0) {
    viewDir = Vec3f(0, 0, -1);
  }
  Vec3f right;
  if (std::abs(viewDir[1]) > 0.999f) {
    right = viewDir.cross(up);
  } else {
    right = viewDir.cross(Vec3f(0, 1, 0));
  }
  up = right.cross(viewDir);
  up.normalize();
  proj = Matrix4f::perspectiveProjection(fovRad, aspect, near, far, false);
  view = Matrix4f::lookAt(eye, at, up);
}

void Camera::VP(float* dst) { 
  //matrix on the right applied first.
  Matrix4f vp = proj * view; 
  const float* ptr = (const float*)(vp);
  for (unsigned i = 0; i < 16; i++) {
    dst[i] = ptr[i];
  }
}

void Camera::VIT(float* dst) {
  Matrix4f vit = view.inverse();
  vit.transpose();
  const float* ptr = (const float*)(vit);
  for (unsigned i = 0; i < 16; i++) {
    dst[i] = ptr[i];
  }
}
