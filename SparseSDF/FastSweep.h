#ifndef FAST_SWEEP_H
#define FAST_SWEEP_H
#include "Array3D.h"
#include "FixedGrid3D.h"

void CloseExterior(Array3D<short>& dist, short far);

/// <summary>
/// 
/// </summary>
/// <param name="dist">distance grid with values in distance unit</param>
/// <param name="voxSize">voxel size in mm</param>
/// <param name="unit">unit for values stored in distance grid in mm</param>
/// <param name="band">narrow band in number of voxels</param>
void FastSweep(Array3D<short>& dist, float voxSize, float unit, float band,
               Array3D<uint8_t>& frozen);
template <unsigned N>
void FastSweep(FixedGrid3D<N> & grid, float voxSize, float unit, float band,
               Array3D<uint8_t>& frozen);

void FastSweepPar(Array3D<short>& dist, float voxSize, float unit, float band,
                  Array3D<uint8_t>& frozen);

void FastSweepParUnsigned(Array3D<short>& dist, float voxSize, float unit, float band,
                          Array3D<uint8_t>& frozen);
#endif