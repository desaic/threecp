#include "GLRender.h"

#define GLEW_STATIC
#include <iostream>

#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "Matrix4f.h"
#include "Matrix3f.h"
#include "Vec4.h"

void PrintGLError() {
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    // Handle the error
    // For example, print an error message
    std::cout<<"gl "<<error<<"\n";
  }
}

std::string GLRender::GLInfo() const {
  std::string s;
  const char* strPtr = (const char*)glGetString(GL_VERSION);
  if (strPtr == nullptr) {
    return "no info";
  }
  s = std::string(strPtr);
  const char* glslVer = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
  std::string glslStr;
  if (glslVer != nullptr) {
    glslStr = std::string(glslVer);
  }
  return s + " glsl: " + glslStr;
}

GLRender::GLRender():_lights(MAX_NUM_LIGHTS) {}

GLRender::~GLRender() {
  // delete texture and framebuffer object
}

/// reallocates texture buffer
void GLRender::Resize(unsigned int width, unsigned int height) {
  if (_width == width && _height == height) {
    return;
  }
  _width = width;
  _height = height;
  //glBindTexture(GL_TEXTURE_2D, _render_tex);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, _render_tex);
  unsigned format = GL_RGBA;
  unsigned type = GL_UNSIGNED_BYTE;
  //glTexImage2D(GL_TEXTURE_2D, 0, format, _width, _height, 0, format, type,
               //nullptr);

  glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, format, _width,
                          _height, true);
  glBindRenderbuffer(GL_RENDERBUFFER, _depth_buffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _width, _height);
  _camera.aspect = width / (float)height;


  glBindTexture(GL_TEXTURE_2D, _resolve_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE,
                _depth_buffer);
  glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_DEPTH_COMPONENT24,
                          _width, _height, GL_TRUE);
}

int GLRender::DrawInstance(size_t instanceId) {
  std::lock_guard<std::mutex> lock(meshLock);
  if (instanceId >= _instances.size()) {
    return -1;
  }
  auto& instance = _instances[instanceId];
  int bufId = instance.bufId;
  if (bufId >= _bufs.size()) {
    return -2;
  }

  GLBuf& buf = _bufs[bufId];
  if (!buf._allocated) {
    AllocateMeshBuffer(bufId);
  }
  if (buf._needsUpdate) {
    UploadMeshData(bufId);
  }
  if (buf._tex.HasImage()) {
    buf._tex.UpdateTextureData();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, buf._tex._texId);
    glUniform1i(_tex_loc, 0); 
  }
  glBindVertexArray(buf.vao);
  glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(3 * buf.mesh->GetNumTrigs()));
  return 0;
}

void GLRender::SetMouse(int x, int y, float wheel, bool left, bool mid,
                        bool right) {
  _mouse.x = x;
  _mouse.y = y;
  //rotate around _camera.at
  if (left) {
    if (_mouse.left) {
      int dx = x - _mouse.oldx;
      int dy = y - _mouse.oldy;
      _mouse.oldx = x;
      _mouse.oldy = y;
      _camera.rotate(xRotSpeed * dx, yRotSpeed * dy);
    } else {
      _mouse.oldx = x;
      _mouse.oldy = y;
    }
  }
  // pan
  else if (right) {
    if (_mouse.right) {
      int dx = x - _mouse.oldx;
      int dy = y - _mouse.oldy;
      _mouse.oldx = x;
      _mouse.oldy = y;
      _camera.pan(camSpeed * dx, camSpeed * dy);
    } else {
      _mouse.oldx = x;
      _mouse.oldy = y;
    }
  } else if (wheel != 0) {
    if (wheel > 0) {
      _camera.zoom(zoomSpeed);
    } else {
      _camera.zoom(-zoomSpeed);
    }
  }
  _mouse.left = left;
  _mouse.mid = mid;
  _mouse.right = right;
}

void GLRender::KeyPressed(KeyboardInput& input) {
  _keyboardInput = input;
  std::vector<std::string> keys = input.splitKeys();
  for (std::string key : keys) {
    if (key == "R" && input.ctrl) {
      ResetCam();
    }
  }
}

int GLRender::UploadLights() {
  unsigned numLights = _lights.NumLights();
  const float eps = 1e-6;
  for (unsigned i = 0; i < numLights; i++) {
    Vec3f worldPos = _lights.world_pos[i];
    Vec4f p(worldPos[0],worldPos[1],worldPos[2], 1.0f);
    p = _camera.view * p;
    if (std::abs(p[3]) < eps) {
      p[3] = eps;
    }
    _lights.pos[i] = Vec3f(p[0] / p[3], p[1] / p[3], p[2] / p[3]);
  }
  glUniform3fv(_lights._pos_loc, _lights.pos.size(),
               (GLfloat*)_lights.pos.data());
  glUniform3fv(_lights._color_loc, _lights.color.size(),
               (GLfloat*)_lights.color.data());
  return 0; 
}

/// renders once
void GLRender::Render() {
  if (!_initialized) {
    return;
  }
  _camera.update();
  glUseProgram(_program);
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glViewport(0, 0, _width, _height);

  glClearColor(clearColor[0], clearColor[1], clearColor[2], 0.8);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  const GLsizei matCount = 1;
  const GLboolean noTranspose = GL_FALSE;
  glUniformMatrix4fv(_pmat_loc, matCount, noTranspose,
                     (const GLfloat*)_camera.proj);

  glUniformMatrix4fv(_vmat_loc, matCount, noTranspose,
                     (const GLfloat*)_camera.view);
  float vit[16];
  _camera.VIT(vit);
  glUniformMatrix4fv(_mvit_loc, matCount, noTranspose, (const GLfloat*)vit);
  UploadLights();
  glEnable(GL_MULTISAMPLE);
  Matrix4f view = _camera.view;  
  for (size_t i = 0; i < _instances.size(); i++) {
    if (_instances[i].hide) {
      continue;
    }
    //multiply model matrix
    Matrix4f model = _instances[i].matrix;
    model = view * model;
    glUniformMatrix4fv(_vmat_loc, matCount, noTranspose, (const GLfloat*)model);
    Matrix4f mvit = model.inverse();
    mvit.transpose();
    glUniformMatrix4fv(_mvit_loc, matCount, noTranspose, (const GLfloat*)mvit);
    DrawInstance(i);
  }
  //restore camera matrix
  glUniformMatrix4fv(_vmat_loc, matCount, noTranspose,
                     (const GLfloat*)_camera.view);  
  glUniformMatrix4fv(_mvit_loc, matCount, noTranspose, (const GLfloat*)vit);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDisable(GL_MULTISAMPLE);
  glUseProgram(0);

  // Resolved multisampling
  glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo_resolve);
  glBlitFramebuffer(0, 0, _width, _height, 0, 0, _width, _height,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int checkShaderError(GLuint shader) {
  GLint success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success == GL_FALSE) {
    GLint logSize = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
    std::vector<GLchar> errorLog(logSize);
    glGetShaderInfoLog(shader, logSize, &logSize, &errorLog[0]);
    std::cout << "Shader error " << shader << " " << errorLog.data() << "\n";
    return -1;
  } else {
    std::cout << "Shader " << shader << " compiled.\n";
    return 0;
  }
}

void GLRender::ResetCam() {
  _camera.eye = _eye0;
  _camera.at = _at0;
  _camera.near = _near0;
  _camera.far = _far0;
  _camera.update();
}

int GLRender::Init(const std::string& vertShader,
                   const std::string& fragShader) {
  _initialized = false;
  _vs_string = vertShader;
  _fs_string = fragShader;
  const char* vs_pointer = _vs_string.data();
  const char* fs_pointer = _fs_string.data();
  ResetCam();
  unsigned err = 0;
  _vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(_vertex_shader, 1, &vs_pointer, NULL);
  glCompileShader(_vertex_shader);
  int ret = checkShaderError(_vertex_shader);
  if (ret < 0) {
    return -1;
  }
  _fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(_fragment_shader, 1, &fs_pointer, NULL);
  glCompileShader(_fragment_shader);
  ret = checkShaderError(_fragment_shader);
  if (ret < 0) {
    return -2;
  }
  _program = glCreateProgram();
  glAttachShader(_program, _vertex_shader);
  glAttachShader(_program, _fragment_shader);
  glLinkProgram(_program);
  int isLinked;
  glGetProgramiv(_program, GL_LINK_STATUS, &isLinked);
  if (isLinked == GL_FALSE) {
    GLint maxLength = 0;
    glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &maxLength);

    // The maxLength includes the NULL character
    std::vector<GLchar> infoLog(maxLength);
    glGetProgramInfoLog(_program, maxLength, &maxLength, &infoLog[0]);
    std::cout << "link shader error " << infoLog.data() << "\n";
  }
  glDetachShader(_program, _vertex_shader);
  glDetachShader(_program, _fragment_shader);
  glDeleteShader(_vertex_shader);
  glDeleteShader(_fragment_shader);
  glUseProgram(_program);
  _vmat_loc = glGetUniformLocation(_program, "View");
  _pmat_loc = glGetUniformLocation(_program, "Proj");
  _mvit_loc = glGetUniformLocation(_program, "MVIT");
  _lights._pos_loc = glGetUniformLocation(_program, "lightpos");
  _lights._color_loc = glGetUniformLocation(_program, "lightcolor");
  _tex_loc = glGetUniformLocation(_program, "texSampler");
  glGenFramebuffers(1, &_fbo);
  glGenFramebuffers(1, &_fbo_resolve);
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);  
  glGenTextures(1, &_render_tex);
  glGenTextures(1, &_resolve_tex);
  glGenTextures(1, &_depth_buffer);
  Resize(INIT_WIDTH, INIT_HEIGHT);
  //glBindTexture(GL_TEXTURE_2D, _render_tex);

  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, _render_tex);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                       _render_tex, 0);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                       _depth_buffer, 0);
  GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, DrawBuffers);
  ret = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (ret != GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "incomplete frame buffer error " << ret << "\n";
  }

  glBindFramebuffer(GL_FRAMEBUFFER, _fbo_resolve);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                       _resolve_tex, 0);
  glDrawBuffers(1, DrawBuffers);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  _initialized = true;
  return 0;
}

int DeleteVAO(GLBuf& buf) {
  std::cout << "delete vao " << buf.vao << "\n";
  PrintGLError();
  glDeleteBuffers(buf.NUM_BUF, buf.b.data());
  PrintGLError();
  glDeleteVertexArrays(1, &buf.vao);  
  PrintGLError();
  return 0;
}

int GLRender::AllocateMeshBuffer(size_t meshId) {
  if (meshId >= _bufs.size()) {
    return -1;
  }
  GLBuf& buf = _bufs[meshId];
  if (buf._allocated) {
    DeleteVAO(buf);
  }
  glGenVertexArrays(1, &buf.vao);
  glBindVertexArray(buf.vao);
  glGenBuffers(GLBuf::NUM_BUF, buf.b.data());
  buf._allocated = true;
  buf._needsUpdate = true;
  buf._numTrigs = buf.mesh->GetNumTrigs();
  return 0;
}

int GLRender::UploadMeshData(size_t meshId) {
  if (meshId >= _bufs.size()) {
    return -1;
  }
  GLBuf& buf = _bufs[meshId];
  if (!buf._allocated || buf._numTrigs != buf.mesh->GetNumTrigs()) {
    AllocateMeshBuffer(meshId);
  }

  auto mesh = buf.mesh;
  if (mesh->nt.size() != mesh->t.size()) {
    mesh->ComputeTrigNormals();
  }
  const unsigned DIM = 3;
  buf._numTrigs = mesh->GetNumTrigs();
  size_t numVerts = mesh->t.size();
  size_t numFloats = DIM * numVerts;
  size_t numUV = 2 * numVerts;
  std::vector<Vec3f> v(numVerts);
  std::vector<Vec3f> n(numVerts);
  std::vector<Vec2f> uv;
  if (mesh->uv.size() != numVerts) {
    // generate uv by placing each triangle on its own pixel to have
    // per-triangle coloring
    unsigned TexWidth = std::sqrt(mesh->GetNumTrigs() + 1) + 1;
    float pixelSize = 1.0f / TexWidth;
    uv.resize(numVerts, Vec2f(0, 0));
    for (unsigned i = 0; i < numVerts; i++) {
      unsigned t = i / 3;
      unsigned row = i / TexWidth;
      unsigned col = i % TexWidth;
      uv[i] = Vec2f((col + 0.5f) * pixelSize, (row + 0.5f) * pixelSize);
    }
  } else {
    uv = mesh->uv;
  }
  for (size_t i = 0; i < mesh->t.size(); i += 3) {
    Vec3f normal = *(Vec3f*)(mesh->nt.data() + i);
    for (int j = 0; j < 3; j++) {
      unsigned vidx = mesh->t[i + j];
      unsigned dstV = i + j;
      v[dstV] = *(Vec3f*)(mesh->v.data() + 3 * vidx);
      n[dstV] = normal;
    }
  }
  glBindVertexArray(buf.vao);
  glBindBuffer(GL_ARRAY_BUFFER, buf.b[0]);
  glBufferData(GL_ARRAY_BUFFER, numFloats * sizeof(GLfloat), v.data(),
               GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer(GL_ARRAY_BUFFER, buf.b[1]);
  glBufferData(GL_ARRAY_BUFFER, numFloats * sizeof(GLfloat), n.data(),
               GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer(GL_ARRAY_BUFFER, buf.b[2]);
  glBufferData(GL_ARRAY_BUFFER, numUV * sizeof(GLfloat), uv.data(),
               GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
  buf._needsUpdate = false;
  return 0;
}

int GLRender::AddInstance(const MeshInstance& inst) {
  int instId = int(_instances.size());
  _instances.push_back(inst);
  return instId;
}

int GLRender::CreateMeshBufs(std::shared_ptr<TrigMesh> mesh) {
  std::lock_guard<std::mutex> lock(meshLock);
  int meshId = int(_bufs.size());
  _bufs.push_back(mesh);
  return meshId;
}

int GLRender::CreateMeshBufAndInstance(std::shared_ptr<TrigMesh> mesh) {
  int bufId = CreateMeshBufs(mesh);
  MeshInstance inst;
  inst.bufId = bufId;
  int instId = AddInstance(inst);
  return instId;
}

int GLRender::SetNeedsUpdate(size_t meshId) {
  if (meshId >= _bufs.size()) {
    return -1;
  }
  _bufs[meshId]._needsUpdate = true;
  return 0;
}

int GLRender::SetMeshTexture(size_t meshId, const Array2D8u& image,
                             int numChannels) {
  if (meshId >= _bufs.size()) {
    return -1;
  }
  Vec2u size = image.GetSize();
  _bufs[meshId]._tex.SetImageData(image.DataPtr(), size[0], size[1],
                                 numChannels);
  return 0;
}

int GLRender::SetSolidColor(size_t meshId, Vec3b color) {
  if (meshId >= _bufs.size()) {
    return -1;
  }
  auto& buf = _bufs[meshId]._tex._buf;
  for (unsigned i = 0; i < buf.size(); i += 4) {
    buf[i] = color[0];
    buf[i + 1] = color[1];
    buf[i + 2] = color[2];
    buf[i + 2] = 255;
  }
  _bufs[meshId]._tex._needUpdate = true;
  return 0;
}

GLBuf::GLBuf(std::shared_ptr<TrigMesh> m) : mesh(m), b(NUM_BUF) {
  _tex.TexWrapParam = GL_REPEAT;
  _tex.MakeDefaultTex();
}