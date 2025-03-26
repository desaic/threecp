#include "Inflate.h"
#include "AdapDF.h"
#include "Stopwatch.h"
#include "AdapSDF.h"
#include "AdapUDF.h"
#include "Array3DUtils.h"
#include "meshutil.h"
#include "Array2D.h"
#include "ImageIO.h"
#include <deque>

float ComputeVoxelSize(const TrigMesh & mesh, float voxResMM, unsigned MAX_GRID_SIZE) {
  BBox box;
  ComputeBBox(mesh.v, box);
  float h = voxResMM;
  for (size_t d = 0; d < 3; d++) {
    float len = box.vmax[d] - box.vmin[d];
    if (len / h > MAX_GRID_SIZE) {
      h = len / MAX_GRID_SIZE;
    }
  }
  return h;
}

static void SaveSlice(const std::string &file, const Array3D<short> & dist, unsigned z, float distUnit) {
  Vec3u size = dist.GetSize();
  Array2D8u slice(size[0], size[1]);
  for (unsigned y = 0; y < size[1]; y++) {
    for (unsigned x = 0; x < size[0]; x++) {
      short val = dist(x, y, z);
      if (val >= 32766) {
        slice(x, y) = 255;
        continue;
      }
      if (val < -32766) {
        slice(x,y) = 0;
        continue;
      }
      float d = val * distUnit;
      if (d > 155) {
        d = 155;
      }
      if (d < -100) {
        d = -100;
      }
      slice(x, y) = uint8_t(100 + d);
    }
  }
  SavePngGrey(file, slice);
}

std::shared_ptr<AdapDF> SignedDistanceFieldBand(const InflateConf& conf,
                                                    TrigMesh& mesh) {
  float h = ComputeVoxelSize(mesh, conf.voxResMM, conf.MAX_GRID_SIZE);
  std::shared_ptr<AdapDF> sdf = std::make_shared<AdapSDF>();
  sdf->distUnit = 1e-2;
  sdf->voxSize = h;
  sdf->band = std::abs(conf.thicknessMM)/ h + 2;
  sdf->mesh_ = &mesh;
  ComputeCoarseDist(sdf.get());  
  return sdf;
}

std::shared_ptr<AdapDF> ComputeSignedDistanceField(const InflateConf& conf,
                                                    TrigMesh& mesh) {
  std::shared_ptr<AdapDF> sdf = SignedDistanceFieldBand(conf, mesh);
  // fast sweep entire grid without narrow band.
  sdf->band = 10000;
  Array3D8u frozen;
  sdf->FastSweepCoarse(frozen);
  SaveSlice("F:/dump/dist.png", sdf->dist, sdf->dist.GetSize()[2] / 2, 0.1);
  return sdf;
}

TrigMesh InflateMesh(const InflateConf& conf, TrigMesh& mesh) {
  std::shared_ptr<AdapDF> sdf = ComputeSignedDistanceField(conf, mesh);
  // SaveSlice(udf->dist, udf->dist.GetSize()[2] / 2, udf->distUnit);
  TrigMesh surf;
  SDFImpAdap dfImp(sdf);
  dfImp.MarchingCubes(conf.thicknessMM, &surf);
  return surf;
}

std::shared_ptr<AdapDF> UnsignedDistanceFieldBand(const InflateConf& conf,
                                                  TrigMesh& mesh) {
  float h = ComputeVoxelSize(mesh, conf.voxResMM, conf.MAX_GRID_SIZE);
  std::shared_ptr<AdapDF> df = std::make_shared<AdapUDF>();
  df->distUnit = 1e-2;
  df->voxSize = h;
  df->band = std::abs(conf.thicknessMM) / h + 2;
  df->mesh_ = &mesh;
  ComputeCoarseDist(df.get());
  return df;
}

std::shared_ptr<AdapDF> ComputeUnsignedDistanceField(const InflateConf& conf,
                                                     TrigMesh& mesh) {
  std::shared_ptr<AdapDF> df = UnsignedDistanceFieldBand(conf, mesh);
  // fast sweep entire grid without narrow band.
  df->band = 10000;
  Array3D8u frozen;
  df->FastSweepCoarse(frozen);
  return df;
}