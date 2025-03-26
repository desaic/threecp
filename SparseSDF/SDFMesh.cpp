#include "SDFMesh.h"

#include <array>
#include <queue>

#include "AdapUDF.h"
#include "MarchingCubes.h"
/// not really infinity. just large enough
#define INF_DIST 1e6f

SDFMesh::SDFMesh() : imp(std::make_shared<SDFImpAdap>(std::make_shared<AdapUDF>())) {}

SDFMesh::SDFMesh(SDFImp* impPtr) { imp = std::shared_ptr<SDFImp>(impPtr); }

void SDFMesh::SetImp(SDFImp* impPtr) { imp = std::shared_ptr<SDFImp>(impPtr); }

float SDFMesh::DistNN(const Vec3f& coord) { return imp->DistNN(coord); }

float SDFMesh::DistTrilinear(const Vec3f& coord) { return imp->DistTrilinear(coord); }

void SDFMesh::MarchingCubes(float level, TrigMesh* surf) { imp->MarchingCubes(level, surf); }

Vec3f SDFMesh::computeGradient(const Vec3f& coord) {
  Vec3f result(0.f, 0.f, 0.f);
  const float dx = imp->conf.voxelSize;
  Vec3f cordR = coord;
  Vec3f cordL = coord;
  // x
  cordR[0] += dx;
  cordL[0] -= dx;
  result[0] = (DistNN(cordR) - DistNN(cordL)) / dx * 0.5f;
  cordR[0] = coord[0];
  cordL[0] = coord[0];

  // y
  cordR[1] += dx;
  cordL[1] -= dx;
  result[1] = (DistNN(cordR) - DistNN(cordL)) / dx * 0.5f;
  cordR[1] = coord[1];
  cordL[1] = coord[1];

  // z
  cordR[2] += dx;
  cordL[2] -= dx;
  result[2] = (DistNN(cordR) - DistNN(cordL)) / dx * 0.5f;
  cordR[2] = coord[2];
  cordL[2] = coord[2];

  if (result.norm2() > 1e-8) result.normalize();

  return result;
}