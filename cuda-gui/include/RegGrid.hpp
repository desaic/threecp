#pragma once

#include <vector>
#include <Eigen/Dense>

//dense 3d integer grid.
struct RegGrid
{
  std::vector<int> gridSize;
  std::vector<int> s;
  Eigen::Vector3d o;
  Eigen::Vector3d dx;
  Eigen::Vector3i lb;
  RegGrid() {
    gridSize.resize(3, 0);
  }
  void allocate(int nx, int ny, int nz) {
    gridSize.resize(3, 0);
    gridSize[0] = nx;
    gridSize[1] = ny;
    gridSize[2] = nz;
    dx[0] = 1.0 / nx;
    dx[1] = 1.0 / ny;
    dx[2] = 1.0 / nz;
    o = Eigen::Vector3d(0, 0, 0);
    lb = Eigen::Vector3i(0, 0, 0);
    int nEle = nx*ny*nz;
    s.resize(nEle, -1);
  }
  int gridToLinear(int i, int j, int k) const {
    return i*gridSize[1] * gridSize[2] + j*gridSize[2] + k;
  }
  int linearToGrid(int l, std::vector<int> & gridIdx) const {
    gridIdx.resize(gridSize.size(), 0);
    for (int i = (int)gridSize.size() - 1; i > 0; i--) {
      gridIdx[i] = l % (gridSize[i]);
      l = l / (gridSize[i]);
    }
    gridIdx[0] = l;
  }
  bool inBound(int i, int j, int k) const {
    return i >= 0 && i < gridSize[0] && j >= 0 && j < gridSize[1] && k >= 0 && k < gridSize[2];
  }

  int operator()(int i, int j, int k) const {
    if (!inBound(i, j, k)) {
      return -1;
    }
    if (i < lb[0] || j < lb[1] || k < lb[2]) {
      return -1;
    }
    return s[gridToLinear(i, j, k)];
  }

  void set(int i, int j, int k, int val) {
    clamp(i, j, k);
    int l = gridToLinear(i, j, k);
    s[l] = val;
  }

  void clamp(int &i, int &j, int & k) const {
    i = std::max(0, i);
    i = std::min(i, gridSize[0] - 1);
    j = std::max(0, j);
    j = std::min(j, gridSize[1] - 1);
    k = std::max(0, k);
    k = std::min(k, gridSize[2] - 1);
  }
};
