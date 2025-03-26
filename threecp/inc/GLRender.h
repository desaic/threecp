#pragma once

#include <mutex>
#include <string>
#include <memory>

#include "Vec3.h"
#include "Matrix4f.h"
#include "Array2D.h"
#include "Camera.h"
#include "GLTex.h"
#include "KeyboardInput.h"
#include "TrigMesh.h"
#include "Scene.h"

/// <summary>
/// Mesh with additional opengl buffers used by shaders.
/// </summary>
struct GLBuf {
  // vertex array object
  unsigned int vao = 0;
  // determined by number of vertex attributes in vertex shader.
  static const int NUM_BUF = 3;
  // vertex buffer
  std::vector<unsigned int> b;
  bool _needsUpdate = false;

  std::shared_ptr<TrigMesh> mesh;
  size_t _numTrigs = 0;
  GLTex _tex;
  GLBuf(std::shared_ptr<TrigMesh> m);
  bool _allocated = false;
};

struct Mouse {
  bool left = false,mid=false, right = false;
  float wheel=0;
  int x = 0;
  int y = 0;
  int oldx = 0;
  int oldy = 0;
};

struct GLLightArray {
  GLLightArray(unsigned numLights)
      : pos( numLights), world_pos( numLights), color( numLights) {}
  void SetLightPos(unsigned li, float x, float y, float z) {
    world_pos[li] = Vec3f(x,y,z);    
  }
  void SetLightColor(unsigned li, float r, float g, float b) {
    color[li] = Vec3f(r,g,b);
  }

  std::vector<Vec3f> pos;
  //light position before transforming by camera matrix.
  std::vector<Vec3f> world_pos;
  std::vector<Vec3f> color;
  int _pos_loc = -1;
  // If your fragment shader does not use the input, it will
  // optimized away with loc = -1
  int _color_loc=-1;
  unsigned NumLights() const { return pos.size(); }
};

struct MeshInstance {
  MeshInstance() : matrix(Matrix4f::identity()) {}
  Matrix4f matrix;
  // index into GLBuf array.
  // also mesh id.
  int bufId = 0;
  int instanceId = 0;
  bool hide = false;
  bool matrixUpdated = false;
};

class GLRender {
 public:
  GLRender();
  ~GLRender();
  /// only works after glfwInit() and being called in the thread
  /// that called glfwInit() because opengl limitations.
  std::string GLInfo() const;

  int Init(const std::string& vertShader, const std::string& fragShader);

  /// reallocates texture buffer
  void Resize(unsigned int width, unsigned int height);

  /// renders once
  void Render();

  void ResetCam();

  unsigned Width() const { return _width; }
  unsigned Height() const { return _height; }
  unsigned TexId() const { return _resolve_tex; }

  /// @brief 
  /// @param mesh 
  /// @return buffer id for new mesh 
  int CreateMeshBufs(std::shared_ptr<TrigMesh> mesh);
  
  /// @brief 
  /// @param bufId index into _bufs
  /// @param inst 
  /// @return global instance id 
  int AddInstance(const MeshInstance &inst);

  /// @brief also creates one instance with identity transformation
  /// @param mesh 
  /// @return new instance id. new buffer id can be queried through instance id.
  int CreateMeshBufAndInstance(std::shared_ptr<TrigMesh> mesh);

  int AllocateMeshBuffer(size_t meshId);

  int DrawInstance(size_t instanceId);

  MeshInstance& GetInstance(int instanceId) { return _instances[instanceId]; }
  const MeshInstance& GetInstance(int instanceId) const {
    return _instances[instanceId];
  }

  void SetMouse(int x, int y, float wheel, bool left, bool mid, bool right);

  void SetPanSpeed(float s) { camSpeed = s; }

  void SetZoomSpeed(float s) { zoomSpeed = s; }

  void KeyPressed(KeyboardInput& keys);

  int SetNeedsUpdate(size_t meshId);

  int SetMeshTexture(size_t meshId, const Array2D8u& image, int numChannels);

  // overwrites existing texture image
  int SetSolidColor(size_t meshId, Vec3b color);

  int UploadLights();

  int UploadMeshData(size_t meshId);

  const static unsigned MAX_NUM_LIGHTS = 8;

  Camera* GetCamera() { return &_camera; }

  GLLightArray* GetLights() { return &_lights; }

  void SetDefaultCameraView(const Vec3f& eye, const Vec3f& at) {
    _eye0 = eye;
    _at0 = at;
  }
  void SetDefaultCameraZ(float near, float far) {
    _near0 = near;
    _far0 = far;
  }

  Vec3f clearColor{0.9, 0.9, 0.9};

 private:
  // rendered
  unsigned _fbo = 0;
  // resolving multisampling
  unsigned _fbo_resolve = 0;
  /// renders to this texture.
  /// because renders from gpu, the cpu buffer in it is unused and empty.
  unsigned _render_tex = 0;
  unsigned _resolve_tex = 0;
  unsigned _depth_buffer = 0;
  Scene scene;

  unsigned int _vertex_shader = 0, _fragment_shader = 0, _program = 0;
  /// view and projection matrix
  /// mvit model view inverse transpose.
  unsigned int _vmat_loc = 0, _pmat_loc = 0, _mvit_loc = 0, _light_loc = 0;
  unsigned int _tex_loc = 0;
  Camera _camera;
  GLLightArray _lights;
  Mouse _mouse;
  bool captureMouse = false;
  float camSpeed = 0.2;
  float xRotSpeed = 4e-3f, yRotSpeed = 4e-3f;
  float zoomSpeed = 5.0f;
  Vec3f _eye0 = {500.0f, 100.0f, 400.0f};
  Vec3f _at0 = {200.0f, 0.0f, 200.0f};
  float _near0 = 1;
  float _far0 = 1000;
  std::string _vs_string, _fs_string;
  std::vector<GLBuf> _bufs;
  std::vector<MeshInstance> _instances;
  const static unsigned INIT_WIDTH = 800, INIT_HEIGHT = 800;
  unsigned _width = 0, _height = 0;
  bool _initialized = false;

  KeyboardInput _keyboardInput;
  std::mutex meshLock;
};
