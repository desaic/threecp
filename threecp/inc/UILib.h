#pragma once

#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "Array2D.h"
#include "GLRender.h"
#include "GLTex.h"
#include "KeyboardInput.h"

namespace ImGui {
class FileBrowser;
};
struct ImGuiIO;
struct GLFWwindow;

class UIWidget {
 public:
  virtual void Draw() {}
  int GetIndex() const {}

 private:
  ///widgets are drawn in order indicated by index_
  int index_ = 0;
};

class Button : public UIWidget {
 public:
  void Draw() override;
  std::function<void()> _onClick;
  // display text+##id
  std::string _text;
  int _id = 0;
  std::string GetUniqueString() const;
  bool sameLine_ = false;
};

class InputInt : public UIWidget {
 public:
  InputInt(const std::string& label, int val) : _label(label), _value(val) {}
  void Draw() override;
  std::string _label;
  int _value;
  bool _entered = false;
};

class InputFloat : public UIWidget {
 public:
  InputFloat(const std::string& label, float val) : _label(label), _value(val) {}
  void Draw() override;
  std::string _label;
  float _value;
  bool _entered = false;
};

class InputText : public UIWidget {
 public:
  InputText(const std::string& label, const std::string& initVal)
      : _label(label) {
    size_t len = std::min(initVal.size(), _value.size()-1);
    std::memcpy(_value.data(), initVal.data(), len); 
    _value[len] = 0;
  }
  std::string GetString()const;
  void Draw() override;
  std::string _label;
  //imgui input text only supports fixed buffer
  std::array<char, 300> _value;
  bool _entered = false;
};

class Label : public UIWidget {
 public:
  std::string _text;
  void Draw() override;
};

class CheckBox : public UIWidget {
 public:
  CheckBox(const std::string& label, bool initVal)
      : label_(label), b_(initVal) {}
  std::string label_;
  void Draw() override;
  bool GetVal() const { return b_; }
 private:
  bool b_;
};

//integer slider.
//integer is all we need.
class Slideri : public UIWidget {
 public:
  Slideri(const std::string& label, int initVal, int lb, int ub)
      : label_(label), i_(initVal),lb_(lb),ub_(ub) {}
  std::string label_;
  void Draw() override;
  int GetVal() const { 
    return i_; 
  }
  void SetVal(int val) {
    if (val < lb_) {
      val = lb_;
    }
    if (val > ub_) {
      val = ub_;
    }
    i_ = val;
  }
  void SetUb(int ub) { ub_ = ub; }
  void SetLb(int lb) { lb_ = lb; }
 private:
  int i_= 0;
  int lb_ = 0;
  int ub_ = 100;
};

class PlotWidget : public UIWidget {
 public:
  PlotWidget(float initMin, float initMax, unsigned w, unsigned h,
             const std::string& title)
      : line_(10, 0),min_(initMin), max_(initMax),
  width_(w), height_(h), title_(title){}
  void SetData(const std::vector<float>& data) {
    std::lock_guard<std::mutex> lock(mtx_);
    line_ = data;
  }
  void Draw() override;
 private:
  std::vector<float> line_;
  float min_ = 0;
  float max_ = 100;
  unsigned width_ = 50;
  unsigned height_ = 50;
  std::string title_ = "";
  //for Draw and SetData
  mutable std::mutex mtx_;
};

/// <summary>
/// floating text uses the DrawList api instead of the normal ImGui api.
/// </summary>
class FloatingText {
 public:
  unsigned color_ = 0x808080ffu;
  std::string text_;  
  Vec2f pos_;
  bool draw_ = true;
};

class UILib {
 public:

  using FileCallback=std::function<void(const std::string&)>;
  using ChangeDirCallback=std::function<void(const std::string&)>;
  using MultiFileCallBack = std::function<void(const std::vector<std::string>&)>;
  using KeyboardCb = std::function<void(KeyboardInput input)>;

  UILib();
  /// launch the ui thread and starts rendering and handling input.
  void Run();
  /// stops the ui thread.
  void Shutdown();

  void UILoop();
  bool IsRunning() const { return _running; }
  ~UILib() { Shutdown(); }

  /// Do not modify buttons in call back or it will deadlock.
  ///@return button id so that its callback or id may be modified in future.
  int AddButton(const std::string& text, const std::function<void()>& onClick,
                bool sameLine = false);

  int SetButtonCallback(int buttonId, const std::function<void()>& onClick);

  int AddCheckBox(const std::string& label, bool initVal);
  int AddSlideri(const std::string& text, int initVal, int lb, int ub);
  int AddLabel(const std::string& text);

  /// @return widget ID.
  int AddImage();

  int AddPlot(float initMin, float initMax,
              const std::string& title);

  int AddWidget(std::shared_ptr <UIWidget> widget);

  int SetPlotData(int widgetId, const std::vector<float>& data);

  /// Can only be called after Init();
  /// If image is RGB, width is data.GetSize()[0]/3.
  int SetImageData(int imageId, const Array2D8u& data, int numChannels);

  int SetImageData(int imageId, const Array2D4b& image);

  int SetMeshTex(int meshId, const Array2D8u& image, int numChannels);
  
  /// @brief set mesh's texture through instance id
  /// @param meshId 
  /// @param image 
  /// @param numChannels 
  /// @return 
  int SetInstTex(int meshId, const Array2D8u& image, int numChannels);
  
  int BufIdFromInst(int instanceId) const;

  int SetMeshNeedsUpdate(int meshId);
  
  int SetImageScale(int id, float scale);

  size_t NumImages() const { return _images.size(); }

  void SetFontsDir(const std::string& dir) { _fontsDir = dir; }

  void SetTitle(const std::string& s) { _title = s; }

  void SetLabelText(int id, const std::string & text);

  /// <summary>
  /// 
  /// </summary>
  /// <param name="text"></param>
  /// <returns>id of the te</returns>
  int AddFloatingText(const std::string& text, unsigned color);
  
  void SetFloatingText(int id, const std::string& text);

  void SetFloatingTextPosition(int id, Vec2f pos);

  bool GetCheckBoxVal(int id) const;
  
  void SetImageWindowTitle(const std::string& s);

  std::string GetGLInfo() const { return _glRender.GLInfo(); }

  GLRender* GetGLRender() { return &_glRender; }

  void SetShaderDir(const std::string& dir) { _shaderDir = dir; }

  /// @return mesh data index
  int AddMesh(std::shared_ptr<TrigMesh> mesh);
  
  /// @return instance id.
  int AddMeshAndInstance(std::shared_ptr<TrigMesh> mesh);

  /// will overwrite existing texture image
  int SetMeshColor(int meshId, Vec3b color);

  void SetWindowSize(unsigned w, unsigned h);

  void SetShowGL(bool show) { _showGL = show; }

  void SetShowImage(bool show) { _showImage = show; }

  void ShowFileOpen(bool multi, const std::string& startingDir) {    
    _openMultiple = multi;
    _showFileOpen = true;
    if (!startingDir.empty()) {
      _pwd = startingDir;
    }
  }

  void SetInitImagePos(int x, int y) {
    _initImagePosX = x;
    _initImagePosY = y;
  }
  void SetFileOpenCb(FileCallback cb) { _onFileOpen = cb; }
  void SetFileSaveCb(FileCallback cb) { _onFileSave = cb; }
  void SetChangeDirCb(ChangeDirCallback cb) {_onChangeDir = cb;}
  void SetMultipleFilesOpenCb(MultiFileCallBack cb) { _onMultipleFilesOpen = cb; }
  
  void SetKeyboardCb(KeyboardCb cb) { _keyboardCb = cb; }

  std::shared_ptr<UIWidget> GetWidget(int id);

 private:

  void DrawImages();
  void DrawFloatingTexts();
  int LoadFonts(ImGuiIO& io);
  int LoadShaders();
  std::thread _uiThread;
  std::array<float, 4> _clearColor = {0.5f, 0.5f, 0.5f, 1};
  GLFWwindow* _window = nullptr;
  int _width = 1280;
  int _height = 900;

  int _initImagePosX = 20;
  int _initImagePosY = 0;

  mutable std::mutex _widgetsLock;
  std::vector<std::shared_ptr<UIWidget> > uiWidgets_;

  std::mutex floatingTextMutex_;
  std::vector<FloatingText> floatingTexts_;

  std::mutex _imagesLock;
  std::vector<GLTex> _images;
  std::shared_ptr<ImGui::FileBrowser> _openDialogue;
  std::shared_ptr<ImGui::FileBrowser> _saveDialogue;
  FileCallback _onFileOpen;
  FileCallback _onFileSave;
  bool _openMultiple = false;
  MultiFileCallBack _onMultipleFilesOpen;
  bool _showFileOpen = false;
  std::string _openFileTitle = "Open...";
  std::string _pwd = "./";
  ChangeDirCallback _onChangeDir;

  std::string _fontsDir = "./fonts";
  std::string _shaderDir = "./glsl/";
  std::string _title;

  std::string imageWindowTitle_="Images";
  bool _showImage = true;
  bool _running = false;

  bool _showGL = true;
  std::mutex glLock_;
  GLRender _glRender;

  KeyboardCb _keyboardCb;
};
