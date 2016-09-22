#include "Render.hpp"
#include <nanogui\nanogui.h>
#include "linmath.h"
#include "FileUtil.hpp"
#include "ElementMesh.hpp"
#include "TrigMesh.hpp"

void Render::drawContent()
{
  mat4x4 m, p, mvp;
  float ratio = 1.0f;
  if (window) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    ratio = width / (float)height;
  }

  mat4x4_identity(m);
  mat4x4_rotate_Y(m, m, (float)glfwGetTime());
  mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
  mat4x4_mul(mvp, p, m);
  glUseProgram(program);
  glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*)mvp);
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
  glDrawArrays(GL_TRIANGLES, 0, 3*m->t.size());
}

void Render::initTrigBuffers(TrigMesh * m)
{
  ShaderBuffer buf;
  glGenVertexArrays(1, &buf.vao);
  glBindVertexArray(buf.vao);
  glGenBuffers(1, buf.b);
  //copy vertices
  glBindBuffer(GL_ARRAY_BUFFER, buf.b[0]);
  //use index buffer instead maybe.
  int dim = 3;
  int nFloat = 3* dim * (int)m->t.size();
  GLfloat * v = new GLfloat[nFloat];
  int cnt= 0;
  for (size_t i = 0; i < m->t.size(); i++) {
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < dim; k++) {
        v[cnt] = m->v[m->t[i][j]][k];
        cnt++;
      }
    }
  }
  glBufferData(GL_ARRAY_BUFFER, nFloat * sizeof(GLfloat), v, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  buffers.push_back(buf);

  delete[]v;
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

  mvp_location = glGetUniformLocation(program, "MVP");

  for (size_t i = 0; i < trigs.size(); i++) {
    initTrigBuffers(trigs[i]);
  }
}

void Render::loadShader(std::string vsfile, std::string fsfile)
{
  vs_string = loadTxtFile(vsfile);
  fs_string = loadTxtFile(fsfile);
}
