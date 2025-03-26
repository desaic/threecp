#ifndef ADAP_UDF_H
#define ADAP_UDF_H

#include "AdapDF.h"

/// <summary>
/// unsigned distance field
/// </summary>
class AdapUDF : public AdapDF {
 public:
  AdapUDF();
  void FastSweepCoarse(Array3D<uint8_t>& frozen) override;
  float MinDist(const std::vector<size_t>& trigs, const std::vector<TrigFrame>& trigFrames,
                const Vec3f& query, const TrigMesh* mesh) override;
};

#endif
