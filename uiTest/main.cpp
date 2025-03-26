#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

#include "UIConf.h"
#include "UILib.h"
#include "Array2D.h"
#include "Vec4.h"

int main(int argc, char** argv) {
  UILib ui;
  UILib* _ui = &ui;
  _ui->SetShowImage(true);

  Array2D<Vec4b> _canvas;
  _canvas.Allocate(800, 800);
  _canvas.Fill(Vec4b(127, 127, 0, 127));
  int _imageId = _ui->AddImage();
  _ui->SetImageData(_imageId, _canvas);
  _ui->AddButton("Run", [&] {

    });
  _ui->AddButton("One Step", [&] {  });
  _ui->AddButton("Stop", [&] {  });
  _ui->AddButton("Snap Pic", [&] {  });
  std::shared_ptr<InputInt> numStepsInput =
    std::make_shared<InputInt>("#steps", 100);
  int _numStepsId = _ui->AddWidget(numStepsInput);
  int _numLabelId = _ui->AddLabel("num: ");
  _ui->SetChangeDirCb([&](const std::string& dir) {  });
  int _statusLabel = _ui->AddLabel("status");

  ui.SetFontsDir("./fonts");

  int statusLabel = ui.AddLabel("status");

  ui.Run();

  const unsigned PollFreqMs = 20;

  while (ui.IsRunning()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(PollFreqMs));
  }
  return 0;
}
