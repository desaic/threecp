#pragma once
#include "FileUtil.hpp"
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

class Render
{
public:
  Render() :window(0){}

  GLuint vertex_shader, fragment_shader, program;
  ///mvp model view projection.
  ///mvit model view inverse transpose.
  GLint mvp_loc, mvit_loc, light_loc;

  GLFWwindow * window;
  std::string vs_string, fs_string;

  std::vector<ElementMesh * > meshes;
  std::vector<TrigMesh * > trigs;
  std::vector<ShaderBuffer> buffers;

  void drawContent();
  void drawElementMesh(ElementMesh * em);
  void drawTrigMesh(TrigMesh * m);
  void init();
  void initTrigBuffers(TrigMesh * m);
  /// \brief loads files into vs_string and fs_string.
  void loadShader(std::string vsfile, std::string fsfile);

};
