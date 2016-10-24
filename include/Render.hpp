#pragma once
#include "FileUtil.hpp"
#include "Camera.hpp"
#include <nanogui\nanogui.h>

class ElementMesh;
class TrigMesh;

struct ShaderBuffer
{
  //vertex array object
  GLuint vao;

  //vertex and color buffer
  std::vector<GLuint> b;

};

struct EMEvent
{
  int eventType;
  std::string filename;
  EMEvent() :eventType(0) {}
};

class Render
{
public:
  Render() :window(0),captureMouse(0),
    camSpeed(1),
    xRotSpeed(4e-3),
  yRotSpeed(4e-3){}

  enum EMEventType {
    NO_EVENT=0,
    //open file event.
    OPEN_FILE_EVENT,
    //moving clipping plane event.
    MOVE_SLICE_EVENT
  };

  GLuint vertex_shader, fragment_shader, program;
  ///mvp model view projection.
  ///mvit model view inverse transpose.
  GLint mvp_loc, mvit_loc, light_loc;
  Camera cam;
  bool captureMouse;
  double xpos0, ypos0;
  float camSpeed;
  float xRotSpeed, yRotSpeed;
  
  GLFWwindow * window;
  std::string vs_string, fs_string;
  std::vector<ElementMesh * > meshes;
  //filenames for meshes.
  std::vector<EMEvent> emEvent;

  std::vector<TrigMesh * > trigs;
  std::vector<ShaderBuffer> buffers;

  void drawContent();
  void elementMeshEvent(int idx);
  void drawElementMesh(ElementMesh * em);
  void drawTrigMesh(TrigMesh * m);
  void init();
  void initTrigBuffers(TrigMesh * m);
  void initEleBuffers(int idx);
  void copyEleBuffers(int idx);
  /// \brief loads files into vs_string and fs_string.
  void loadShader(std::string vsfile, std::string fsfile);

  void moveCamera(float dt);
};
