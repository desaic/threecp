///\file
#pragma once

#include <string>
#include <vector>
#include <iostream>

int loadArr3dTxt(std::string filename, std::vector<double> & arr,
  int &inputSize);

void loadBinaryStructure(const std::string & filename,
  std::vector<int> & s,
  std::vector<int> & gridSize);

void loadBinaryStructure(const std::string & filename,
  std::vector<double> & s,
  std::vector<int> & gridSize);

void loadBinDouble(const std::string & filename,
  std::vector<double>&s,
  std::vector<int> & gridSize);

void printIntStructure(const double * s, const std::vector<int> & gridSize,
  std::ostream & out);

void saveBinaryStructure(const std::string & filename,
  const std::vector<int> & s,
  const std::vector<int> & gridSize);


void saveBinaryStructure(const std::string & filename,
  const std::vector<double> & s,
  const std::vector<int> & gridSize);

std::vector<double>
mirrorOrthoStructure(const std::vector<double> &s, std::vector<int> & gridSize);

std::vector<double> repeatStructure(const std::vector<double> &s, std::vector<int> & gridSize,
  int nx, int ny, int nz);

///\brief zero out everything except values in the tet used to mirror cubic structures.
///the first tet covers x,y,z<=0.5, x>=y>=z.
void getCubicTet(std::vector<double> &s, std::vector<int> & gridSize);
