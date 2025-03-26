#pragma once
#ifndef UICONF_H
#define UICONF_H
#include <string>

class UIConf {
 public:
  /// loads from _confFile if confFile is empty
  int Load(const std::string& confFile);
  int Save();
  void SetWorkingDir(const std::string& dir) { workingDir = dir; }
  std::string _confFile = "./uiconf.json";
  int _numTokens = 128;
  std::string outputFile = "grid.txt";
  float voxResMM = 0.032f;
  float waxGapMM = 0.064f;
  float thicknessMM = 0.5f;
  std::string workingDir = "./";
  std::string outDir = "./";
};
#endif