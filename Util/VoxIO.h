#pragma once
#include "Array3d.h"
#include <string>

void SaveVoxTxt(const Array3D8u& grid, float voxRes, const std::string& filename);