#pragma once
#include "FileUtil.hpp"
#include "Camera.hpp"
#include <nanogui\nanogui.h>
#include "RegGrid.hpp"
#include <set>

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
  EMEvent() :eventType(0), slice(0) {}
};

struct Ray
{
  Eigen::Vector3f o, d;
};

typedef std::map<std::pair<int, int>, int> EdgeMap;
typedef std::set<int> VertexSet;
struct PickEvent
{
  //ray used for picking.
  Ray r;
  bool picked;
  ShaderBuffer buf;
  VertexSet verts;
  EdgeMap edges;  
  std::vector<int> selection;
  //# of lines to render.
  int nLines;
  int graphIdx;
  PickEvent() :picked(0), nLines(0), graphIdx(0) {
    selection.resize(2, -1);
  }
  void saveGraph(ElementMesh * em, RegGrid * grid);
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
void copyRayBuffers(ShaderBuffer & buf, PickEvent & event, ElementMesh * em);
