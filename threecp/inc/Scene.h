#pragma once
#ifndef SCENE_H
#define SCENE_H

#include "Camera.h"
#include "TrigMesh.h"

struct LightSrc {
  enum TYPE { POINT, DIRECTION, RECT };
  TYPE type;
  Vec3f x;
  Vec3f dir;
};

class Scene {
 
public:
  
  Camera camera;

  std::vector<TrigMesh> meshes;

  std::vector<LightSrc> lights;

  void AddGround();

  void InitCamera();
};

#endif