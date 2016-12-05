#include "AddPillar.hpp"
#include "ArrayUtil.hpp"

//does not include i,j
std::vector<int> neighborHood(int i, int j, float radius, 
  int sizei, int sizej)
{
  std::vector <int> N;
  int bound = (int)radius + 1;
  for (int ni = i - bound; ni <= i + bound; ni++) {
    for (int nj = j - bound; nj <= j + bound; nj++) {
      if (ni == i && nj == j) { continue; }
      if (!inbound(ni, nj, sizei, sizej)) { continue; }
      int nidx = gridToLinearIdx(ni, nj, sizej);
      float dist = (ni - i)*(ni - i) + (nj - j)*(nj - j);
      if (dist <= radius * radius) {        
        N.push_back(nidx);
      }
    }
  }
  return N;
}

std::vector<double> addPillar(std::vector<double>& arr, std::vector<int>& gridSize)
{
  std::vector<double> support(arr.size(), 0);

  float matThresh = 0.5f;
  int layerSize = gridSize[1] * gridSize[2];
  //maximum overhang size in number of voxels.
  float radius = 1;
  float pillarRadius = 3;

  for (int k = 1; k < gridSize[2]; k++) {
    std::vector<int> supported(layerSize, 0);
    //find supported voxels.
    for (int i = 0; i < gridSize[0]; i++) {
      for (int j = 0; j < gridSize[1]; j++) {
        int lidx = gridToLinearIdx(i, j, k, gridSize);
        if (arr[lidx] < matThresh) { continue; }
        int below = gridToLinearIdx(i, j, k-1, gridSize);
        if (arr[below] > matThresh) {
          int lidx2d = gridToLinearIdx(i, j, gridSize[1]);
          supported[lidx2d] = 1;
          std::vector<int> N = neighborHood(i, j, radius, gridSize[0], gridSize[1]);
          for (int ni = 0; ni < (int)N.size(); ni++) {
            supported[ni] = 1;
          }
        }       
      }
    }

    //generate pillars for unsupported voxels.
    for (int i = 0; i < gridSize[0]; i++) {
      for (int j = 0; j < gridSize[1]; j++) {
        int lidx = gridToLinearIdx(i, j, k, gridSize);
        int lidx2d = gridToLinearIdx(i, j, gridSize[1]);
        //skip void
        if (arr[lidx] < matThresh) { continue; }
        //skip supported voxels.
        if (supported[lidx2d] > 0) { continue; }
        std::vector<int> N1 = neighborHood(i, j, (pillarRadius + radius), gridSize[0], gridSize[1]);
        int supCount = 0;
        int materialCount = 0;
        float minDist = -1;
        for (int ni = 0; ni < (int)N1.size(); ni++) {
          if (supported[N1[ni]] == 0) {
            continue;
          }
          int lidx = N1[ni] * gridSize[2] + k;
          std::vector<int> gridIdx;
          linearToGridIdx(lidx, gridSize, gridIdx);
          Eigen::Vector2d disp(gridIdx[0] - i, gridIdx[1] - j);
          double dist = disp.squaredNorm();
          if (dist < minDist || minDist<0) {
            minDist = dist;
          }
        }

        if (minDist > 0 ){
          continue;
        }

        std::vector<int> N = neighborHood(i, j, pillarRadius, gridSize[0], gridSize[1]);
        N.push_back(gridToLinearIdx(i, j, gridSize[1]));

        for (int ni = 0; ni < (int)N.size(); ni++) {
          //shoot ray downwards until it hits the floor or some material.
          for (int ri = k - 1; ri >= 0; ri--) {
            //convert 2d grid cell to 3d grid cell idx.
            int cellIdx = N[ni] * gridSize[2] + ri;
            if (arr[cellIdx] >= matThresh || support[cellIdx] >= matThresh) {
              break;
            }
            support[cellIdx] = 1.0f;
          }
          supported[N[ni]] = 1;
        }
        
        //mark nearby unsupported cells as supported
        for (int ni = 0; ni < (int)N1.size(); ni++) {
          int lidx2d = N1[ni];
          int cellIdx = N1[ni] * gridSize[2] + k;
          if (supported[lidx2d] == 0 && arr[cellIdx]>=matThresh) {
            supported[lidx2d] = 1;
          }
        }
      }
    }

  }
  return support;
}
