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
  int slice;
  EMEvent() :eventType(0) {}
};

struct Ray
{
  Eigen::Vector3f o, d;
};

struct PickEvent
{
  //ray used for picking.
  Ray r;
  bool picked;
  ShaderBuffer buf;
  PickEvent() :picked(0) {}
};

//dense 3d integer grid.
struct RegGrid
{
  std::vector<int> gridSize;
  std::vector<int> s;
  Eigen::Vector3d o;
  Eigen::Vector3d dx;
  void allocate(int nx, int ny, int nz) {
    gridSize.resize(3, 0);
    gridSize[0] = nx;
    gridSize[1] = ny;
    gridSize[2] = nz;
    dx[0] = 1.0 / nx;
    dx[1] = 1.0 / ny;
    dx[2] = 1.0 / nz;
    o = Eigen::Vector3d(0, 0, 0);
    int nEle = nx*ny*nz;
    s.resize(nEle, -1);
  }
  int gridToLinear(int i, int j, int k) const {
    return i*gridSize[1] * gridSize[2] + j*gridSize[2] + k;
  }
  int linearToGrid(int l, std::vector<int> & gridIdx) const {
    gridIdx.resize(gridSize.size(), 0);
    for (int i = (int)gridSize.size() - 1; i > 0; i--) {
      gridIdx[i] = l % (gridSize[i]);
      l = l / (gridSize[i]);
    }
    gridIdx[0] = l;
  }
  bool inBound(int i, int j, int k) const {
    return i >= 0 && i < gridSize[0] && j >= 0 && j < gridSize[1] &&k >= 0 && k < gridSize[2];
  }
  
  int operator()(int i, int j, int k) const {
    if (!inBound(i, j, k)) {
      return -1;
    }
    return s[gridToLinear(i, j, k)];
  }

  void set(int i, int j, int k, int val) {
    clamp(i, j, k);
    int l = gridToLinear(i, j, k);
    s[l] = val;
  }

  void clamp(int &i, int &j, int & k) {
    i = std::max(0, i);
    i = std::min(i, gridSize[0] - 1);
    j = std::max(0, j);
    j = std::min(j, gridSize[1] - 1);
    k = std::max(0, k);
    k = std::min(k, gridSize[2] - 1);
  }
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

  PickEvent pickEvent;
  RegGrid grid;

  void drawContent();
  void elementMeshEvent(int idx);
  void drawElementMesh(ElementMesh * em);
  void drawTrigMesh(TrigMesh * m);
  void init();
  void initTrigBuffers(TrigMesh * m);
  void initEleBuffers(int idx);
  void initRayBuffers();
  void copyEleBuffers(int idx);
  void updateGrid(const std::vector<double> & s, const std::vector<int> & gridSize);
  void pick(double xpos, double ypos);

  /// \brief loads files into vs_string and fs_string.
  void loadShader(std::string vsfile, std::string fsfile);

  void moveCamera(float dt);
};

int rayGridIntersect(const Ray & r0, const RegGrid & grid);
bool rayBoxIntersect(const Ray & r, const Eigen::Vector3f * bounds,
  float & tmin, float & tmax);
