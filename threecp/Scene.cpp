#include "Scene.h"

void MakePlane(TrigMesh & m) {
  m.v = {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0};
  m.t = {0,1,2,2,3,0};
}

void Scene::AddGround() {
  TrigMesh plane;
  const float groundSize = 200;
  plane.scale(groundSize);
  MakePlane(plane);
  meshes.push_back(plane);
}

void Scene::InitCamera() {
  
}