#include "Render.hpp"
#include <nanogui\nanogui.h>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include "FileUtil.hpp"
#include "ElementHex.hpp"
#include "ElementMesh.hpp"
#include "ElementMeshUtil.hpp"
#include "ElementRegGrid.hpp"
#include "ParametricStructure3D.hpp"
#include "TrigMesh.hpp"
#include "linmath.h"
#include "VoxelIO.hpp"

void Render::updateGrid(const std::vector<double> & s, const std::vector<int> & gridSize)
{
  grid.allocate(gridSize[0], gridSize[1], gridSize[2]);
  int cnt = 0;
  float thresh = 0.5;
  for (int j = 0; j < s.size(); j++) {
    if (s[j] < thresh) {
      grid.s[j] = -1;
    }
    else {
      grid.s[j] = cnt;
      cnt++;
    }
  }
}

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
    
    loadBinaryStructure(emEvent[idx].filename, s, gridSize);
    s = mirrorOrthoStructure(s, gridSize);
    if (gridSize.size() == 0) {
      break;
    }
    em = new ElementRegGrid();
    assignGridMat(s, gridSize, em);
    updateGrid(s, gridSize);
    if (idx == 0) {
      pickEvent.edges.clear();
      pickEvent.selection[0] = -1;
    }
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
    //std::cout << "#ele " << meshes[i]->e.size() << "\n";
  }
  for (size_t i = 0; i < trigs.size(); i++) {
    glBindVertexArray(buffers[i+meshes.size()].vao);
    drawTrigMesh(trigs[i]);
  }
  if (pickEvent.picked && pickEvent.nLines>0 && meshes.size()>0) {
    glBindVertexArray(pickEvent.buf.vao);
    int nEdges = (int)pickEvent.edges.size();
    glDrawArrays(GL_TRIANGLES, 0, 36 * pickEvent.nLines);
  }
  if (g.E.size() > 0) {
    glBindVertexArray(gbuf.vao);
    glDrawArrays(GL_TRIANGLES, 0, 36 * g.E.size());
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
  Eigen::Vector3f color = e->color[ei];
  if (e->eLabel[ei] == 1) {
    color = Eigen::Vector3f(0.5, 0.9, 0.5);
  }
  else if (e->eLabel[ei] == 2) {
    color = Eigen::Vector3f(0.9, 0.5, 0.5);
  }
  Eigen::Vector3d center = eleCenter(e, ei);
  std::vector<Eigen::Vector3d> verts(e->e[ei]->nV());
  for (int i = 0; i < e->e[ei]->nV(); i++) {
    verts[i]=center + (e->X[e->e[ei]->at(i)]-center);
  }
  cnt = addCube(verts, color, v, n, c);
  return cnt;
}

void Render::copyEleBuffers(int idx)
{
  if ((int)meshes.size() <= idx) {
    return;
  }
  std::cout << "Copy em buffer " << idx << "\n";
  ShaderBuffer buf = buffers[idx];
  ElementMesh * e = meshes[idx];
  std::cout << "em #ele " << e->e.size() << "\n";
  int slice = emEvent[idx].slice;
  grid.lb[2] = slice;
  int gridres = grid.gridSize[2];
  double dx = 1.0 / gridres;

  std::vector<int> eidx;
  for (size_t i = 0; i < e->e.size(); i++) {
    Eigen::Vector3d center = eleCenter(e, i);
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

int addLine(Eigen::Vector3d x0, Eigen::Vector3d x1,
  const Eigen::Vector3f & color,
  GLfloat * v, GLfloat * n, GLfloat * c)
{
  std::vector < Eigen::Vector3d> verts(8);
  Eigen::Vector3d x(1, 0, 0);
  Eigen::Vector3d d = x1 - x0;
  float dz = d.norm();
  Eigen::Vector3d z = (1.0 / dz)*d;
  if (std::abs(z[0]) > 0.9) {
    x << 0, 0, 1;
  }
  Eigen::Vector3d y = z.cross(x);
  Eigen::Vector3d o = x0;
  y.normalize();
  x = y.cross(z);
  x.normalize();
  
  float dx = 0.01;
  verts[0] = o - dx * x - dx * y;
  verts[1] = o - dx * x - dx * y + dz * z;
  verts[2] = o - dx * x + dx * y;
  verts[3] = o - dx * x + dx * y + dz * z;
  verts[4] = o + dx * x - dx * y;
  verts[5] = o + dx * x - dx * y + dz * z;
  verts[6] = o + dx * x + dx * y;
  verts[7] = o + dx * x + dx * y + dz * z;

  int cnt = addCube(verts, color, v, n, c);
  return cnt;
}

int addCuboid(Cuboid & cuboid,
  const Eigen::Vector3f & color,
  GLfloat * v, GLfloat * n, GLfloat * c)
{
  std::vector < Eigen::Vector3d> verts(8);
  Eigen::Vector3d x0(cuboid.x0[0], cuboid.x0[1], cuboid.x0[2]);
  Eigen::Vector3d x1(cuboid.x1[0], cuboid.x1[1], cuboid.x1[2]);
  Eigen::Vector3d x = x1 - x0;
  float len = x.norm();
  if (len < 1e-10) {
    //draw something random if d is nearly 0.
    x = Eigen::Vector3d(1e-5, 1e-5, 1e-5);
    len = x.norm();
  }
  x = x / len;
  Eigen::Vector3d y(0, 1, 0);
  if (std::abs(x[1]) > 0.9) {
    y = Eigen::Vector3d(1, 0, 0);
  }
  Eigen::Vector3d z = x.cross(y);
  z.normalize();
  y = z.cross(x);
  Eigen::Matrix3d R = Eigen::AngleAxisd(cuboid.theta, x).toRotationMatrix();
  y = R*y;
  z = R*z;

  float dy = cuboid.r[0];
  float dz = cuboid.r[1];
  verts[0] = x0 - dy * y - dz * z;
  verts[1] = x0 - dy * y + dz * z;
  verts[2] = x0 + dy * y - dz * z;
  verts[3] = x0 + dy * y + dz * z;
  verts[4] = x0 + len * x - dy * y - dz * z;
  verts[5] = x0 + len * x - dy * y + dz * z;
  verts[6] = x0 + len * x + dy * y - dz * z;
  verts[7] = x0 + len * x + dy * y + dz * z;

  int cnt = addCube(verts, color, v, n, c);
  return cnt;
}

void copyRayBuffers(ShaderBuffer & buf, PickEvent & event, ElementMesh * em)
{
  
  Eigen::Vector3f color;
  color << 0.8, 0.7, 0.7;
  int nTrig = 12;
  int dim = 3;
  event.nLines = event.edges.size();
  if (event.nLines <= 0) {
    return;
  }
  int nFloat = nTrig * event.nLines * 3 * dim;
  GLfloat * v = new GLfloat[nFloat];
  GLfloat * n = new GLfloat[nFloat];
  GLfloat * c = new GLfloat[nFloat];
  float len = 2;
  Eigen::Vector3d x0 = event.r.o.cast<double>();
  Eigen::Vector3d x1 = x0 + len * event.r.d.cast<double>();
  int cnt = 0;
  
  for (EdgeMap::const_iterator iter = event.edges.begin(); iter != event.edges.end(); ++iter) {
    std::pair<int,int> k = iter->first;
    int e1 = k.first;
    int e2 = k.second;
    Eigen::Vector3d c1 = eleCenter(em, e1);
    Eigen::Vector3d c2 = eleCenter(em, e2);
    cnt += addLine(c1, c2, color, v + cnt, n + cnt, c + cnt);
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
  glBufferData(GL_ARRAY_BUFFER, nFloat * sizeof(GLfloat), c, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

  delete[]v;
  delete[]n;
  delete[]c;
}

void Render::copyGraphBuffers()
{
  Eigen::Vector3f basecolor[2];
  basecolor[0] << 0.2, 0.2, 1;
  basecolor[1] << 1, 0.2, 0.2;
  int nTrig = 12;
  int dim = 3;
  if (cuboids.size() == 0) {
    return;
  }
  int nFloat = nTrig * g.E.size() * 3 * dim;
  GLfloat * v = new GLfloat[nFloat];
  GLfloat * n = new GLfloat[nFloat];
  GLfloat * c = new GLfloat[nFloat];
  float len = 2;
  int cnt = 0;

  for (size_t i = 0; i < cuboids.size(); i++) {
    float a = (i / (float)(cuboids.size()-1));
    Eigen::Vector3f color =  a * basecolor[1] + (1-a) * basecolor[0];
    cnt += addCuboid(cuboids[i], color, v + cnt, n + cnt, c + cnt);
  }

  glBindVertexArray(gbuf.vao);
  glBindBuffer(GL_ARRAY_BUFFER, gbuf.b[0]);
  glBufferData(GL_ARRAY_BUFFER, nFloat * sizeof(GLfloat), v, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer(GL_ARRAY_BUFFER, gbuf.b[1]);
  glBufferData(GL_ARRAY_BUFFER, nFloat * sizeof(GLfloat), n, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer(GL_ARRAY_BUFFER, gbuf.b[2]);
  glBufferData(GL_ARRAY_BUFFER, nFloat * sizeof(GLfloat), c, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

  delete[]v;
  delete[]n;
  delete[]c;
}

void Render::initGraphBuffers()
{
  int nBuf = 3;
  glGenVertexArrays(1, &gbuf.vao);
  glBindVertexArray(gbuf.vao);
  gbuf.b.resize(nBuf, 0);
  glGenBuffers(nBuf, gbuf.b.data());
  copyGraphBuffers();
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
  if (meshes.size() > 0) {
    copyRayBuffers(pickEvent.buf, pickEvent, meshes[0]);
  }
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
  if (cuboids.size() > 0) {
    initGraphBuffers();
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
  if (grid.gridSize[0] > 0 && meshes.size()>0) {
    int id = rayGridIntersect(pickEvent.r, grid);
    if (id >= 0) {
      meshes[0]->eLabel[id] = (meshes[0]->eLabel[id] + 1) % 3;
      std::cout << "Pick " << id << "\n";
      copyEleBuffers(0);
      //making an edge
      if (meshes[0]->eLabel[id] == 2) {
        if (pickEvent.selection[0] < 0) {
          pickEvent.selection[0] = id;
        }
        else {
          if (pickEvent.selection[0] != id) {
            pickEvent.selection[1] = id;
            pickEvent.edges[std::make_pair(pickEvent.selection[0], pickEvent.selection[1])] = 1;
            std::fill(pickEvent.selection.begin(), pickEvent.selection.end(), -1);
          }
        }
      }
      else if(meshes[0]->eLabel[id] == 1){
        pickEvent.verts.insert(id);
      }
      else if(meshes[0]->eLabel[id] == 0){
        pickEvent.verts.erase(id);
      }
    }
  }
  if (meshes.size() > 0) {
    copyRayBuffers(pickEvent.buf, pickEvent, meshes[0]);
  }
}

void PickEvent::saveGraph(ElementMesh * em, RegGrid * grid) {
  std::string filename = "graph" + std::to_string(graphIdx) + ".txt";
  std::cout << "save graph " << filename << "\n";
  FileUtilOut out(filename);
  out.out << verts.size() << " " << edges.size() << "\n";
  //out.out << grid->gridSize[0] << " " << grid->gridSize[1] << " " << grid->gridSize[2] << "\n";
  //for (std::set<int>::const_iterator it = verts.begin(); it != verts.end(); it++) {
  //  out.out << *it << "\n";
  //}
  std::map<int, int> verts;
  for (EdgeMap::const_iterator iter = edges.begin(); iter != edges.end(); ++iter){
    std::pair<int, int> k = iter->first;
    int e1 = k.first;
    int e2 = k.second;
    if (verts.find(e1) == verts.end()) {
      verts[e1] = verts.size();
    }
    if(verts.find(e2) == verts.end()){
      verts[e2] = verts.size();
    }
  }
  for (std::map<int, int>::const_iterator it = verts.begin(); it != verts.end(); it++) {
    int ei = it->first;
    Eigen::Vector3d center = eleCenter(em, ei);
    out.out << ei << " " << it->second;
    for (int j = 0; j < 3; j++) {
      out.out << " " << center[j];
    }
    out.out << "\n";
  }

  for (EdgeMap::const_iterator iter = edges.begin(); iter != edges.end(); ++iter){
    std::pair<int, int> k = iter->first;
    int e1 = k.first;
    int e2 = k.second;
    out.out << verts[e1] << " " << verts[e2] << "\n";
  }
  out.close();
  graphIdx++;
}
