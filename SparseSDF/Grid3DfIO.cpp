#include "Grid3DfIO.h"

#include <iomanip>

namespace slicer {

static const std::string ORIGIN = "origin";
static const std::string VOXEL_SIZE = "voxelSize";
static const std::string GRID = "grid";

template<typename T>
void PrintVec3(std::ostream& out, const Vec3<T>& v) {
  out << v[0] << " " << v[1] << " " << v[2];
}

void SaveTxt(const Grid3Df& grid, std::ostream& out, int precision) {
  if (precision > 0 && precision < 20) {
    out << std::setprecision(precision);
  }
  out << ORIGIN << "\n";
  PrintVec3(out, grid.origin);
  out << "\n";
  out << VOXEL_SIZE << "\n";
  PrintVec3(out, grid.voxelSize);
  out << "\n";
  out << GRID << "\n";
  PrintVec3(out, grid.GetSize());
  out << "\n";
  Vec3u size = grid.GetSize();
  for (unsigned z = 0; z < size[2]; z++) {
    for (unsigned y = 0; y < size[1]; y++) {
      for (unsigned x = 0; x < size[0]; x++) {
        out << grid(x, y, z) << " ";
      }
      out << "\n";
    }    
  }
}

Grid3Df LoadTxt(std::istream& in) { 
  std::string name;
  Grid3Df grid;
  in >> name;
  in >> grid.origin[0] >> grid.origin[1] >> grid.origin[2];
  in >> name;
  in >> grid.voxelSize[0] >> grid.voxelSize[1] >> grid.voxelSize[2];
  in >> name;
  Vec3u gridSize(0);
  in >> gridSize[0] >> gridSize[1] >> gridSize[2];
  const size_t MAX_GRID_SIZE = 1000000;
  for (size_t d = 0; d < 3; d++) {
    //invalid grid size
    if (gridSize[d] == 0 || gridSize[d] >= MAX_GRID_SIZE) {
      return Grid3Df();
    }
  }
  grid.Allocate(gridSize, 0);
  for (unsigned z = 0; z < gridSize[2]; z++) {
    for (unsigned y = 0; y < gridSize[1]; y++) {
      for (unsigned x = 0; x < gridSize[0]; x++) {
        in >> grid(x, y, z);
      }
    }
  }
  return grid;
}

}  // namespace slicer
