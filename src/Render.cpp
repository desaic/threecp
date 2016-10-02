#include "Render.hpp"
#include <nanogui\nanogui.h>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include "FileUtil.hpp"
#include "ElementMesh.hpp"
#include "TrigMesh.hpp"
#include "linmath.h"

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
    drawElementMesh(meshes[i]);
  }
  for (size_t i = 0; i < trigs.size(); i++) {
    glBindVertexArray(buffers[i].vao);
    drawTrigMesh(trigs[i]);
  }
}

void Render::drawElementMesh(ElementMesh * em)
{
  
}

void Render::drawTrigMesh(TrigMesh * m)
{
  glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(3*m->t.size()));
}

void Render::initTrigBuffers(TrigMesh * m)
{
  ShaderBuffer buf;
  int nBuf = 2;
  glGenVertexArrays(1, &buf.vao);
  glBindVertexArray(buf.vao);
  buf.b.resize(nBuf);
  glGenBuffers(nBuf, buf.b.data());

  //use index buffer instead maybe.
  int dim = 3;
  int nFloat = 3* dim * (int)m->t.size();
  GLfloat * v = new GLfloat[nFloat];
  GLfloat * n = new GLfloat[nFloat];
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

  buffers.push_back(buf);

  delete[]v;
  delete[]n;
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
  cam.eye << 0, 0.5, 2;
  cam.at << 0, 0, -2;
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
  for (size_t i = 0; i < trigs.size(); i++) {
    initTrigBuffers(trigs[i]);
  }
}

void Render::loadShader(std::string vsfile, std::string fsfile)
{
  vs_string = loadTxtFile(vsfile);
  fs_string = loadTxtFile(fsfile);
}
