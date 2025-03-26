#pragma once

#include <memory>

#include "Array3D.h"
#include "TrigMesh.h"

class AdapDF;
struct InflateConf {
  float thicknessMM = 1.0f;
  float voxResMM = 0.5f;
  unsigned MAX_GRID_SIZE = 1000;
};

float ComputeVoxelSize(const TrigMesh& mesh, float voxResMM,
                       unsigned MAX_GRID_SIZE);
std::shared_ptr<AdapDF> SignedDistanceFieldBand(const InflateConf& conf,
                                                    TrigMesh& mesh);
std::shared_ptr<AdapDF> ComputeSignedDistanceField(const InflateConf& conf,
                                                 TrigMesh& mesh);
std::shared_ptr<AdapDF> UnsignedDistanceFieldBand(const InflateConf& conf,
                                                TrigMesh& mesh);
std::shared_ptr<AdapDF> ComputeUnsignedDistanceField(const InflateConf& conf,
                                                   TrigMesh& mesh);
TrigMesh InflateMesh(const InflateConf& conf, TrigMesh& mesh);
Array3D8u FloodOutside(const Array3D<short>& dist, float distThresh);