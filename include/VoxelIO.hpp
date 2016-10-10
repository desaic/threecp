///\file
#pragma once

#include <string>
#include <vector>

void loadBinaryStructure(const std::string & filename,
  std::vector<int> & s,
  std::vector<int> & gridSize);

void loadBinaryStructure(const std::string & filename,
  std::vector<double> & s,
  std::vector<int> & gridSize);

void loadBinDouble(const std::string & filename,
  std::vector<double>&s,
  std::vector<int> & gridSize);

void saveBinaryStructure(const std::string & filename,
  const std::vector<int> & s,
  const std::vector<int> & gridSize);


void saveBinaryStructure(const std::string & filename,
  const std::vector<double> & s,
  const std::vector<int> & gridSize);
