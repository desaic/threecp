#ifndef ADAP_SDF_H
#define ADAP_SDF_H

#include "AdapDF.h"

/// <summary>
/// signed distance field.
/// </summary>
class AdapSDF : public AdapDF {
 public:
  AdapSDF();
  float MinDist(const std::vector<size_t>& trigs, const std::vector<TrigFrame>& trigFrames,
                const Vec3f& query, const TrigMesh* mesh) override;
};

#endif
