#include "Render.hpp"
#include <nanogui\nanogui.h>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include "FileUtil.hpp"
#include "ElementHex.hpp"
#include "ElementMesh.hpp"
#include "ElementMeshUtil.hpp"
#include "ElementRegGrid.hpp"
#include "TrigMesh.hpp"
#include "linmath.h"
#include "VoxelIO.hpp"

void Render::elementMeshEvent(int idx)
{
  int eventType = emEvent[idx].eventType;
  if (eventType != NO_EVENT) {
    emEvent[idx].eventType = NO_EVENT;
  }
  else {
    return ;
  }
  std::vector<double> s;
  std::vector<int> gridSize;
  ElementRegGrid * em = 0;
  switch (eventType) {
  case OPEN_FILE_EVENT:
    em = new ElementRegGrid();
    loadBinaryStructure(emEvent[idx].filename, s, gridSize);
    assignGridMat(s, gridSize, em);
    delete meshes[idx];
    meshes[idx] = em;
    copyEleBuffers(idx);
    break;
  case MOVE_SLICE_EVENT:
    copyEleBuffers(idx);
    break;
  }

}

void Render::drawContent()
{
  Eigen::Matrix4f v, mvp, mvit;
  if (window) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    cam.ratio = width / (float)height;
  }
  
  v = Eigen::Matrix4f::Identity();
  Eigen::Vector3f axis;
  axis << 0, 1, 0;
  float angle = (float)glfwGetTime();
  Eigen::AngleAxis<float> rot((float)glfwGetTime(), axis);
  cam.update();
  v = mat4x4_look_at(cam.eye, cam.at, cam.up);
  mvp = cam.p*v;
  mvit = v.inverse().transpose();
  glUseProgram(program);
  glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, (const GLfloat*)mvp.data());
  glUniformMatrix4fv(mvit_loc, 1, GL_FALSE, (const GLfloat*)mvit.data());
  for (size_t i = 0; i < meshes.size(); i++) {
    elementMeshEvent(i);
    glBindVertexArray(buffers[i].vao);
    drawElementMesh(meshes[i]);
  }
  for (size_t i = 0; i < trigs.size(); i++) {
    glBindVertexArray(buffers[i+meshes.size()].vao);
    drawTrigMesh(trigs[i]);
  }
  if (pickEvent.picked) {
    glBindVertexArray(pickEvent.buf.vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
  }
}

void Render::drawElementMesh(ElementMesh * em)
{
  glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(36 * em->e.size()));
}

void Render::drawTrigMesh(TrigMesh * m)
{
  glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(3*m->t.size()));
}

void Render::initTrigBuffers(TrigMesh * m)
{
  ShaderBuffer buf;
  int nBuf = 3;
  glGenVertexArrays(1, &buf.vao);
  glBindVertexArray(buf.vao);
  buf.b.resize(nBuf);
  glGenBuffers(nBuf, buf.b.data());

  //use index buffer instead maybe.
  int dim = 3;
  int nFloat = 3* dim * (int)m->t.size();
  GLfloat * v = new GLfloat[nFloat];
  GLfloat * n = new GLfloat[nFloat];
  GLfloat * color = new GLfloat[nFloat];
  int cnt= 0;
  m->compute_norm();
  for (size_t i = 0; i < m->t.size(); i++) {
    Vector3s trigv[3];
    for (int j = 0; j < 3; j++) {
      trigv[j] = m->v[m->t[i][j]];
    }
    Vector3s normal = (trigv[1] - trigv[0]).cross(trigv[2] - trigv[0]);
    normal.normalize();
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < dim; k++) {
        v[cnt] = (GLfloat)m->v[m->t[i][j]][k];
        n[cnt] = (GLfloat)normal[k];
        color[cnt] = (GLfloat)0.7;
        //n[cnt] = (GLfloat)m->n[m->t[i][j]][k];
        cnt++;
      }
    }
  }
  glBindBuffer(GL_ARRAY_BUFFER, buf.b[0]);
  glBufferData(GL_ARRAY_BUFFER, nFloat * sizeof(GLfloat), v, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer(GL_ARRAY_BUFFER, buf.b[1]);
  glBufferData(GL_ARRAY_BUFFER, nFloat * sizeof(GLfloat), n, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer(GL_ARRAY_BUFFER, buf.b[2]);
  glBufferData(GL_ARRAY_BUFFER, nFloat * sizeof(GLfloat), color, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

  buffers.push_back(buf);

  delete[]v;
  delete[]n;
  delete[]color;
}


int addCube(const std::vector<Eigen::Vector3d> & verts,
  const Eigen::Vector3f & color, 
  GLfloat * v, GLfloat * n, GLfloat * c)
{
  int cnt = 0;
  int nQuad = 6;
  int nTrig = 2;
  int dim = 3;
  int quadT[2][3] = { { 0,1,2 },{ 0,2,3 } };
  for (int qi = 0; qi < nQuad; qi++) {
    for (int ti = 0; ti < nTrig; ti++) {
      Vector3s trigv[3];
      int vidx[3];
      for (int j = 0; j < 3; j++) {
        vidx[j] = hexFaces[qi][quadT[ti][j]];
        trigv[j] = verts[vidx[j]];
      }
      Vector3s normal = (trigv[1] - trigv[0]).cross(trigv[2] - trigv[0]);
      normal.normalize();
      for (int j = 0; j < 3; j++) {
        for (int k = 0; k < dim; k++) {
          v[cnt] = (GLfloat)trigv[j][k];
          n[cnt] = (GLfloat)normal[k];
          c[cnt] = (GLfloat)color[k];
          cnt++;
        }
      }

    }
  }
  return cnt;
}

int addHexEle(ElementMesh * e, int ei, 
  GLfloat * v, GLfloat * n, GLfloat * c)
{
  int cnt = 0;
  int dim = 3;
  Eigen::Vector3f color;
  color << e->color[ei], e->color[ei], e->color[ei];
  std::vector<Eigen::Vector3d> verts(e->e[ei]->nV());
  for (int i = 0; i < e->e[ei]->nV(); i++) {
    verts[i]=e->X[e->e[ei]->at(i)];
  }
  cnt = addCube(verts, color, v, n, c);
  return cnt;
}

void Render::copyEleBuffers(int idx)
{
  ShaderBuffer buf = buffers[idx];
  ElementMesh * e = meshes[idx];
  int slice = emEvent[idx].slice;
  int gridres = 32;
  double dx = 1.0 / gridres;

  std::vector<int> eidx;
  for (size_t i = 0; i < e->e.size(); i++) {
    Eigen::Vector3d center(0, 0, 0);
    int nV = (int)e->e[i]->nV();
    for (int j = 0; j < nV; j++) {
      center += e->X[e->e[i]->at(j)];
    }
    center = (1.0 / nV) * center;
    if (center[2] >= slice* dx) {
      eidx.push_back(i);
    }
  }
  if (eidx.size() == 0) {
    return;
  }
  //use index buffer instead maybe.
  int dim = 3;
  int nTrig = 12;
  int nFloat = nTrig * 3 * dim * (int)eidx.size();
  GLfloat * v = new GLfloat[nFloat];
  GLfloat * n = new GLfloat[nFloat];
  GLfloat * color = new GLfloat[nFloat];
  int cnt = 0;

  for (size_t i = 0; i < (int)eidx.size(); i++) {
    int ret = addHexEle(e, eidx[i], v + cnt, n + cnt, color + cnt);
    cnt += ret;
  }
  glBindVertexArray(buf.vao);
  glBindBuffer(GL_ARRAY_BUFFER, buf.b[0]);
  glBufferData(GL_ARRAY_BUFFER, nFloat * sizeof(GLfloat), v, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer(GL_ARRAY_BUFFER, buf.b[1]);
  glBufferData(GL_ARRAY_BUFFER, nFloat * sizeof(GLfloat), n, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer(GL_ARRAY_BUFFER, buf.b[2]);
  glBufferData(GL_ARRAY_BUFFER, nFloat * sizeof(GLfloat), color, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
    
  delete[]v;
  delete[]n;
  delete[]color;
}

void Render::initEleBuffers(int idx)
{
  ElementMesh* e = meshes[idx];
  ShaderBuffer buf;
  int nBuf = 3;
  glGenVertexArrays(1, &buf.vao);
  glBindVertexArray(buf.vao);
  buf.b.resize(nBuf);
  glGenBuffers(nBuf, buf.b.data());
  buffers.push_back(buf);
  copyEleBuffers(idx);
}

void copyRayBuffers(ShaderBuffer & buf, const Ray & r)
{
  std::vector < Eigen::Vector3d> verts(8);

  Eigen::Vector3d x(1, 0, 0);
  if (std::abs(r.d[0]) > 0.9) {
    x << 0, 0, 1;
  }
  Eigen::Vector3d z = r.d.cast<double>();
  Eigen::Vector3d y = z.cross(x);
  Eigen::Vector3d o = r.o.cast<double>();
  y.normalize();
  x = y.cross(z);
  x.normalize();
  float dz = 2;
  float dx = 0.005;
  verts[0] = o - dx * x - dx * y;
  verts[1] = o - dx * x - dx * y + dz * z;
  verts[2] = o - dx * x + dx * y;
  verts[3] = o - dx * x + dx * y + dz * z;
  verts[4] = o + dx * x - dx * y;
  verts[5] = o + dx * x - dx * y + dz * z;
  verts[6] = o + dx * x + dx * y;
  verts[7] = o + dx * x + dx * y + dz * z;

  Eigen::Vector3f color;
  color << 0.8, 0.7, 0.7;
  int nTrig = 12;
  int dim = 3;
  int nFloat = nTrig * 3 * dim;
  GLfloat * v = new GLfloat[nFloat];
  GLfloat * n = new GLfloat[nFloat];
  GLfloat * c = new GLfloat[nFloat];

  int cnt = addCube(verts, color, v, n, c);
  glBindVertexArray(buf.vao);
  glBindBuffer(GL_ARRAY_BUFFER, buf.b[0]);
  glBufferData(GL_ARRAY_BUFFER, nFloat * sizeof(GLfloat), v, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer(GL_ARRAY_BUFFER, buf.b[1]);
  glBufferData(GL_ARRAY_BUFFER, nFloat * sizeof(GLfloat), n, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer(GL_ARRAY_BUFFER, buf.b[2]);
  glBufferData(GL_ARRAY_BUFFER, nFloat * sizeof(GLfloat), c, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

  delete[]v;
  delete[]n;
  delete[]c;
}

void Render::initRayBuffers()
{
  int nBuf = 3;
  glGenVertexArrays(1, &pickEvent.buf.vao);
  glBindVertexArray(pickEvent.buf.vao);
  pickEvent.buf.b.resize(nBuf);
  glGenBuffers(nBuf, pickEvent.buf.b.data());
  pickEvent.r.o = Eigen::Vector3f(0, 0, 0);
  pickEvent.r.d = Eigen::Vector3f(0, 0, 1);
  copyRayBuffers(pickEvent.buf, pickEvent.r);
}

void checkShaderError(GLuint shader)
{
  GLint success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success == GL_FALSE) {
    GLint logSize = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
    std::vector<GLchar> errorLog(logSize);
    glGetShaderInfoLog(shader, logSize, &logSize, &errorLog[0]);
    std::cout << "Shader error " << shader << " " << errorLog.data() << "\n";
  }
  else {
    std::cout << "Shader " << shader << " compiled.\n";
  }
}

void Render::init()
{
  const char * vs_pointer = vs_string.data();
  const char * fs_pointer = fs_string.data();
  cam.angle_xz = 0;
  cam.eye << 0, 0.5, -2;
  cam.at << 0, 0, 2;
  cam.update();
  std::cout<<"glsl version"<<glGetString(GL_SHADING_LANGUAGE_VERSION)<<"\n";
  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vs_pointer, NULL);
  glCompileShader(vertex_shader);
  checkShaderError(vertex_shader);

  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fs_pointer, NULL);
  glCompileShader(fragment_shader);
  checkShaderError(fragment_shader);

  program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  glDetachShader(program, vertex_shader);
  glDetachShader(program, fragment_shader);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  mvp_loc = glGetUniformLocation(program, "MVP");
  mvit_loc = glGetUniformLocation(program, "MVIT");
  
  emEvent.resize(meshes.size());
  for (size_t i = 0; i < meshes.size(); i++) {
    initEleBuffers(i);
  }
  for (size_t i = 0; i < trigs.size(); i++) {
    initTrigBuffers(trigs[i]);
  }
  initRayBuffers();
}

void Render::loadShader(std::string vsfile, std::string fsfile)
{
  vs_string = loadTxtFile(vsfile);
  fs_string = loadTxtFile(fsfile);
}

void Render::moveCamera(float dt)
{
  Eigen::Vector3f viewDir = cam.at - cam.eye;
  Eigen::Vector3f up = cam.up;
  Eigen::Vector3f right = viewDir.cross(up);
  right[1] = 0;
  viewDir[1] = 0;

  if (cam.keyhold[0]) {
    cam.eye += viewDir * dt * camSpeed;
    cam.at += viewDir * dt * camSpeed;
  }
  if (cam.keyhold[1]) {
    cam.eye -= viewDir * dt * camSpeed;
    cam.at -= viewDir * dt * camSpeed;
  }
  if (cam.keyhold[2]) {
    cam.eye += right * dt * camSpeed;
    cam.at += right * dt * camSpeed;
  }
  if (cam.keyhold[3]) {
    cam.eye -= right * dt * camSpeed;
    cam.at -= right * dt * camSpeed;
  }
  if (cam.keyhold[4]) {
    if (cam.eye[1]<2) {
      cam.eye[1] += dt * camSpeed;
      cam.at[1] += dt * camSpeed;
    }
  }
  if (cam.keyhold[5]) {
    if (cam.eye[1]>-0.5) {
      cam.eye[1] -= dt * camSpeed;
      cam.at[1] -= dt * camSpeed;
    }
  }
}

void Render::pick(double xpos, double ypos)
{
  int width=0, height=0;
  if (window) {
    glfwGetFramebufferSize(window, &width, &height);
  }
  else {
    return;
  }
  pickEvent.picked = true;
  Eigen::Vector4f d;
  d << 2 * xpos / width -1, 1 - 2 * ypos / height, 1 , 1;
  d = cam.p.inverse()*d;
  pickEvent.r.o = cam.eye;
  d[3] = 0;
  Eigen::Matrix4f vmat = mat4x4_look_at(cam.eye, cam.at, cam.up);
  pickEvent.r.d = (vmat.transpose() * d).block(0,0,3,1);
  pickEvent.r.d.normalize();
  copyRayBuffers(pickEvent.buf, pickEvent.r);
}
