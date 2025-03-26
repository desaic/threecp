#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

#include "CadApp.hpp"
#include "CanvasApp.h"
#include "InflateApp.h"
#include "PackApp.hpp"
#include "UIConf.h"
#include "UILib.h"
#include "VoxApp.h"
#include "HeadApp.hpp"

void VoxApp() {
  UILib ui;
  ConnectorVox app;
  app.Init(&ui);
  ui.SetFontsDir("./fonts");

  int statusLabel = ui.AddLabel("status");

  ui.Run();

  const unsigned PollFreqMs = 20;

  while (ui.IsRunning()) {
    app.Refresh();
    std::this_thread::sleep_for(std::chrono::milliseconds(PollFreqMs));
  }
}

void RunInflateApp() {
  UILib ui;
  InflateApp app;
  app.Init(&ui);
  ui.SetFontsDir("./fonts");
  int statusLabel = ui.AddLabel("status");
  ui.Run();
  const unsigned PollFreqMs = 20;
  while (ui.IsRunning()) {
    app.Refresh();
    std::this_thread::sleep_for(std::chrono::milliseconds(PollFreqMs));
  }
}

void RunCadApp() {
  UILib ui;
  CadApp app;
  app.Init(&ui);
  ui.SetFontsDir("./fonts");
  int statusLabel = ui.AddLabel("status");
  ui.Run();

  const unsigned PollFreqMs = 20;

  while (ui.IsRunning()) {
    app.Refresh();
    std::this_thread::sleep_for(std::chrono::milliseconds(PollFreqMs));
  }
}

void RunCanvasApp() {
  UILib ui;
  CanvasApp app;
  app.conf._confFile = "./canvas_conf.json";
  app.Init(&ui);
  ui.SetFontsDir("./fonts");

  int statusLabel = ui.AddLabel("status");

  ui.Run();

  const unsigned PollFreqMs = 20;

  while (ui.IsRunning()) {
    app.RefreshUI();
    std::this_thread::sleep_for(std::chrono::milliseconds(PollFreqMs));
  }
}

void RunPackApp() {
  UILib ui;
  PackApp app;
  app.Init(&ui);
  ui.SetFontsDir("./fonts");
  int statusLabel = ui.AddLabel("status");
  ui.Run();

  const unsigned PollFreqMs = 20;

  while (ui.IsRunning()) {
    app.Refresh();
    std::this_thread::sleep_for(std::chrono::milliseconds(PollFreqMs));
  }
}

extern void MapDrawingToThickness();

void RunHeadApp() {
  UILib ui;
  HeadApp app;
  app.Init(&ui);
  ui.SetFontsDir("./fonts");

  int statusLabel = ui.AddLabel("status");

  ui.Run();

  const unsigned PollFreqMs = 20;

  while (ui.IsRunning()) {
    app.Refresh();
    std::this_thread::sleep_for(std::chrono::milliseconds(PollFreqMs));
  }
}

extern void MeshHeightMap();
extern void MapDrawingToThickness();
extern void TestSDF();

extern void SaveVolMesh();

int main(int argc, char** argv) {
  // RunCanvasApp();
  // RunInflateApp();
  // RunPackApp();
  // VoxApp();
  //RunHeadApp();
  //MeshHeightMap();
  //MapDrawingToThickness();
  TestSDF();
   //SaveVolMesh();
  return 0;
}
