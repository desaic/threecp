#pragma once
#include "FileUtil.hpp"
#include <nanogui\nanogui.h>

class ElementMesh;
class TrigMesh;

static const char* default_vs_text =
"uniform mat4 MVP;\n"
"attribute vec3 vCol;\n"
"attribute vec3 vPos;\n"
"varying vec3 color;\n"
"void main()\n"
"{\n"
"    gl_Position = MVP * vec4(vPos, 1.0);\n"
"    color = vCol;\n"
"}\n";

static const char* default_fs_text =
"varying vec3 color;\n"
"void main()\n"
"{\n"
"    gl_FragColor = vec4(color, 1.0);\n"
"}\n";

struct Vertex
{
  float x, y,z;
  float r, g, b;
};

struct ShaderBuffer
{
  //vertex array object
  GLuint vao;

  //vertex and color buffer
  GLuint b[2];

};

class Render
{
public:
  Render() :window(0), vs_string(default_vs_text),
  fs_string(default_fs_text){}

  GLuint vertex_shader, fragment_shader, program;
  GLint mvp_location, vpos_location, vcol_location;

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
