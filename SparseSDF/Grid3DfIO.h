#pragma once

#include <iostream>
#include "Grid3Df.h"
namespace slicer {

  void SaveTxt(const Grid3Df& grid, std::ostream& out, int precision = 6);

  Grid3Df LoadTxt(std::istream& in);

}  // namespace slicer
