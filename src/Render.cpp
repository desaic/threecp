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
    break;
  }

}

void Render::drawContent()
{
  Eigen::Matrix4f m, v, p, mvp, mvit;
  float ratio = 1.0f;
  if (window) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    ratio = width / (float)height;
  }
  
  m = Eigen::Matrix4f::Identity();
  v = Eigen::Matrix4f::Identity();
  Eigen::Vector3f axis;
  axis << 0, 1, 0;
  float angle = (float)glfwGetTime();
  Eigen::AngleAxis<float> rot((float)glfwGetTime(), axis);
  //m.block(0, 0, 3, 3) = rot.matrix();
  //m(3, 3) = 1;
  cam.update();
  v = mat4x4_look_at(cam.eye, cam.at, cam.up);
  p = mat4x4_perspective(3.14f/3, ratio, 0.1f, 20);
  mvp = p*v*m;
  mvit = (v*m).inverse().transpose();
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

int addHexEle(ElementMesh * e, int ei, 
  GLfloat * v, GLfloat * n, GLfloat * color)
{
  int nQuad = 6;
  int nTrig = 2;
  int quadT[2][3] = { {0,1,2},{0,2,3} };
  int cnt = 0;
  int dim = 3;
  for (int qi = 0; qi < nQuad; qi++) {
    for (int ti = 0; ti < nTrig; ti++) {
      Vector3s trigv[3];
      int vidx[3];
      for (int j = 0; j < 3; j++) {
        Element*ele = e->e[ei];
        vidx[j] = hexFaces[qi][quadT[ti][j]];
        trigv[j] = e->X[ele->at(vidx[j])];
      }
      Vector3s normal = (trigv[1] - trigv[0]).cross(trigv[2] - trigv[0]);
      normal.normalize();
      for (int j = 0; j < 3; j++) {
        for (int k = 0; k < dim; k++) {
          v[cnt] = (GLfloat)trigv[j][k];
          n[cnt] = (GLfloat)normal[k];
          color[cnt] = (GLfloat)e->color[ei];
          //n[cnt] = (GLfloat)m->n[m->t[i][j]][k];
          cnt++;
        }
      }

    }
  }
  return cnt;
}

void Render::copyEleBuffers(int idx)
{
  ShaderBuffer buf = buffers[idx];
  ElementMesh * e = meshes[idx];

  //use index buffer instead maybe.
  int dim = 3;
  int nTrig = 12;
  int nFloat = nTrig * 3 * dim * (int)e->e.size();
  GLfloat * v = new GLfloat[nFloat];
  GLfloat * n = new GLfloat[nFloat];
  GLfloat * color = new GLfloat[nFloat];
  int cnt = 0;

  for (size_t i = 0; i < e->e.size(); i++) {
    int ret = addHexEle(e, i, v + cnt, n + cnt, color + cnt);
    cnt += ret;
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

void checkShaderError(GLuint shader)
{
  GLint success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success == GL_FALSE) {
    GLint logSize = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
    std::vector<GLchar> errorLog(logSize);
    glGetShaderInfoLog(shader, logSize, &logSize, &errorLog[0]);
    std::cout << "Shader error " << shader << " " << errorLog.data()<<"\n";
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