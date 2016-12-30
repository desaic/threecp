#include "ParametricStructure3D.hpp"
#include "ArrayUtil.hpp"
#include "EigenUtil.hpp"
#include <iostream>
#include <algorithm>
#include <Eigen/Dense>

void 
ParametricStructure3D::resize(int nx, int ny, int nz)
{
  if (nx <= 0 || ny <= 0 ||nz<=0){
    std::cout << "Parametric structure N<=0.\n";
    return;
  }
  gridSize.resize(3);
  gridSize[0] = nx;
  gridSize[1] = ny;
  gridSize[2] = nz;
  int nEle = nx * ny * nz;
  s.resize(nEle);
}

ParametricNPR3D::ParametricNPR3D(){
  gridSize.resize(3, 0);

  core.r = 0.04;
  core.x0[0] = 0.5;
  core.x0[1] = 0.3;
  core.x0[2] = core.x0[1];
  core.x1[0] = 0.4;
  core.x1[1] = 0.4;
  core.x1[2] = 0.25;

  c.r = 0.05;
  c.x0[0] = 0.5 - c.r - 0.04;
  c.x0[1] = c.r;
  c.x0[2] = 0;
  c.x1[0] = 0.22;
  c.x1[1] = 0.5 - c.x1[0];
  c.x1[2] = 0;

  c1.r = 0.08;
  c1.x0[0] = 0.45;
  c1.x0[1] = 0;
  c1.x0[2] = 0;
  c1.x1[0] = 0;
  c1.x1[1] = 0;
  c1.x1[2] = 0;

  con1.r = 0.03;
  con1.x0[0] = core.x0[0];
  con1.x0[1] = core.x0[1] + con1.r;
  con1.x0[2] = core.x0[2];
  con1.x1[0] = 0.3;
  con1.x1[1] = 0.23;
  con1.x1[2] = con1.x1[1] - con1.r;

  con2.r = 0.03;
  con2.x0[0] = con1.x1[0];
  con2.x0[1] = con1.x1[1];
  con2.x0[2] = con1.x1[2];
  con2.x1[0] = c.x0[0] - con2.r;
  con2.x1[1] = c.x0[1];
  con2.x1[2] = c.x0[2];
}

//distribute parameters into geometric primitives.
void ParametricNPR3D::distributeParam()
{
  if (param.size() < 7){
    std::cout << "param npr 3d wrong param size " << param.size() << "\n";
  }
  //6 params. c.x1[0]  c.r c1.r con1.r con1.x1[0] con1.x1[1]
  //core.r
  //con1.x1
  //con2.r
  core.r = param[0];
  c.x1[0] = param[1];
  c.r = param[2];
  c1.r = param[3];
  con1.r = param[4];
  con1.x1[0] = param[5];
  con1.x1[1] = param[6];
  con2.r = param[7];

  c.x1[1] = 0.5 - c.x1[0];
  con1.x1[2] = con1.x1[1] - con1.r;
  con2.x0[0] = con1.x1[0];
  con2.x0[1] = con1.x1[1];
  con2.x0[2] = con1.x1[2];
  con2.x1[0] = c.x0[0] - con2.r;
  con2.x1[1] = c.x0[1];
  con2.x1[2] = c.x0[2];
}

//read variable parameters from geometric primitives
void ParametricNPR3D::extractParam()
{
  param.resize(8);
  param[0] = core.r;
  param[1] = c.x1[0];
  param[2] = c.r;
  param[3] = c1.r;
  param[4] = con1.r;
  param[5] = con1.x1[0];
  param[6] = con1.x1[1];
  param[7] = con2.r;
}

void ParametricNPR3D::eval(const std::vector<double> & p)
{
  param = p;
  distributeParam();
  std::fill(s.begin(), s.end(), 0.0);
  drawCylinder(core, s, gridSize, 1.0);
  drawCylinder(c, s, gridSize, 1.0);
  drawCylinder(c1, s, gridSize, 1.0);
  drawCylinder(con1, s, gridSize, 1.0);
  drawCylinder(con2, s, gridSize, 1.0);
  s = mirrorCubicStructure(s, gridSize);
}

/// \param t if pt is between the 2 end points, t is between 0 and 1.
double ptLineDist(Eigen::Vector3d pt, Eigen::Vector3d x0, Eigen::Vector3d x1,
  double & t)
{
  Eigen::Vector3d v0 = pt - x0;
  Eigen::Vector3d dir = (x1 - x0);
  double len = dir.norm();
  dir = (1.0 / len)*dir;
  //component of v0 parallel to the line segment.
  t = v0.dot(dir);
  Eigen::Vector3d a = t * dir;
  t = t / len;
  Eigen::Vector3d b = v0 - a;
  double dist = 0;
  if (t < 0){
    dist = v0.norm();
  }
  else if (t > 1){
    dist = (pt - x1).norm();
  }else{
    dist = b.norm();
  }
  return dist;
}

void drawCylinder(Cylinder3D & cy, std::vector<double> & arr,
  const std::vector<int> & gridSize, double val)
{
  //bounding box of the cylinder.
  float fl[3];
  float fu[3];

  int il[3];
  int iu[3];

  for (int i = 0; i < 3; i++){
    fl[i] = std::min(cy.x0[i], cy.x1[i]) - cy.r;
    fl[i] = std::max(0.0f, fl[i]);
    il[i] = (int)(fl[i] * gridSize[i] + 0.5);
    fu[i] = std::max(cy.x0[i], cy.x1[i]) + cy.r;
    fu[i] = std::min(1.0f, fu[i]);
    iu[i] = (int)(fu[i] * gridSize[i] + 0.5);
  }
  Eigen::Vector3d x0, x1;
  x0 = floatToEigen3d(cy.x0);
  x1 = floatToEigen3d(cy.x1);
  for (int i = il[0]; i <= iu[0]; i++){
    for (int j = il[1]; j <= iu[1]; j++){
      for (int k = il[2]; k <= iu[2]; k++){
        if (!inbound(i, j, k, gridSize)){
          continue;
        }
        Eigen::Vector3d coord;
        coord << (i+0.5)/gridSize[0], (j+0.5)/gridSize[1], (k+0.5)/gridSize[2];
        double t = 0;
        double dist = ptLineDist(coord, x0, x1, t);
        if (dist <= cy.r &&t>=0 && t<=1){
          int idx = gridToLinearIdx(i, j, k, gridSize);          
          arr[idx] = val;
        }
      }
    }
  }
}

void drawCylinder2(Cylinder3D2 & cy, std::vector<double> & arr,
  const std::vector<int> & gridSize, double val)
{
  //bounding box of the cylinder.
  float fl[3];
  float fu[3];

  int il[3];
  int iu[3];
  float r = std::max(cy.r[0], cy.r[1]);
  for (int i = 0; i < 3; i++){
    fl[i] = std::min(cy.x0[i], cy.x1[i]) - r;
    fl[i] = std::max(0.0f, fl[i]);
    il[i] = (int)(fl[i] * gridSize[i] + 0.5);
    fu[i] = std::max(cy.x0[i], cy.x1[i]) + r;
    fu[i] = std::min(1.0f, fu[i]);
    iu[i] = (int)(fu[i] * gridSize[i] + 0.5);
  }
  Eigen::Vector3d x0, x1;
  x0 = floatToEigen3d(cy.x0);
  x1 = floatToEigen3d(cy.x1);
  
  for (int i = il[0]; i <= iu[0]; i++){
    for (int j = il[1]; j <= iu[1]; j++){
      for (int k = il[2]; k <= iu[2]; k++){
        if (!inbound(i, j, k, gridSize)){
          continue;
        }
        Eigen::Vector3d coord;
        coord << (i + 0.5) / gridSize[0], (j + 0.5) / gridSize[1], (k + 0.5) / gridSize[2];
        double t = 0;
        double dist = ptLineDist(coord, x0, x1, t);
        float r = (1 - t) * cy.r[0] + t*cy.r[1];
        if (dist <= r &&t >= 0 && t <= 1){
          int idx = gridToLinearIdx(i, j, k, gridSize);
          arr[idx] = val;
        }
      }
    }
  }
}

void drawCuboid(const Cuboid & c, std::vector<double> & arr,
  const std::vector<int> & gridSize, double val)
{
  Eigen::Vector3d v0 = floatToEigen3d(c.x0);
  Eigen::Vector3d v1 = floatToEigen3d(c.x1);
  Eigen::Vector3d x = v1-v0;
  float len = x.norm();
  if (len < 1e-10){
    return;
  }
  x = (1.0 / len) * x;
  //default arbitrary y axis.
  Eigen::Vector3d y(0,1,0);
  if ( std::abs(x[1]) > 0.9 ){
    y = Eigen::Vector3d(1, 0, 0);
  }
  Eigen::Vector3d z = x.cross(y);
  z.normalize();
  y = z.cross(x);
  Eigen::Matrix3d R = Eigen::AngleAxisd(c.theta, x).toRotationMatrix();
  y = R*y;
  z = R*z;

  double r = std::sqrt(std::pow(c.r[0], 2) + std::pow(c.r[1], 2));
  double r_avg = 0.5 * (c.r[0] + c.r[1]);
  int dim = 3;

  //bounding box of the cylinder.
  float fl[3];
  float fu[3];

  int il[3];
  int iu[3];
  for (int i = 0; i < 3; i++){
    fl[i] = std::min(c.x0[i], c.x1[i]) - r;
    fl[i] = std::max(0.0f, fl[i]);
    il[i] = (int)(fl[i] * gridSize[i] + 0.5);
    fu[i] = std::max(c.x0[i], c.x1[i]) + r;
    fu[i] = std::min(1.0f, fu[i]);
    iu[i] = (int)(fu[i] * gridSize[i] + 0.5);
  }

  Eigen::Vector3d x0 = floatToEigen3d(c.x0);
  Eigen::Vector3d x1 = floatToEigen3d(c.x1);
  for (int i = il[0]; i <= iu[0]; i++){
    for (int j = il[1]; j <= iu[1]; j++){
      for (int k = il[2]; k <= iu[2]; k++){
        if (!inbound(i, j, k, gridSize)){
          continue;
        }
        Eigen::Vector3d coord;
        coord << (i + 0.5) / gridSize[0], (j + 0.5) / gridSize[1], (k + 0.5) / gridSize[2];
        double t = 0;
        double dist = ptLineDist(coord, x0, x1, t);
        Eigen::Vector3d disp = coord - x0;
        float ycoord = std::abs(disp.dot(y));
        float zcoord = std::abs(disp.dot(z));

        if (dist <= r_avg && ycoord <= c.r[0] && zcoord <= c.r[1]){
          int idx = gridToLinearIdx(i, j, k, gridSize);
          arr[idx] = val;
        }
      }
    }
  }
}

void ParamShear3D::distributeParam()
{

}

void
ParamShear3D::extractParam()
{

}

void ParamShear3D::eval(const std::vector<double> & p)
{
  param = p;
  distributeParam();
  std::fill(s.begin(), s.end(), 0.0);
  drawCylinder(c, s, gridSize, 1.0);
  drawCylinder(c1, s, gridSize, 1.0);
  drawCylinder(core, s, gridSize, 1.0);
  drawCylinder(con1, s, gridSize, 1.0);
  //drawCylinder(con2, s, gridSize, 1.0);
  s = mirrorCubicStructure(s, gridSize);
}

NPR3D1::NPR3D1()
{
  core.r = 0.1;
  core.x0[0] = 0.5;
  core.x0[1] = 0.5;
  core.x0[2] = 0.5;
  core.x1[0] = 0.3;
  core.x1[1] = 0.3;
  core.x1[2] = 0.3;

  c1.r = 0.03;
  c1.x0[0] = core.x1[0];
  c1.x0[1] = core.x1[1];
  c1.x0[2] = core.x1[2];
  c1.x1[0] = 0.4;
  c1.x1[1] = 0.4-c1.r;
  c1.x1[2] = 0;
  
  c2.r = 0.03;
  c2.x0[0] = c1.x1[0];
  c2.x0[1] = c1.x1[1];
  c2.x0[2] = c1.x1[2];
  c2.x1[0] = 0;
  c2.x1[1] = 0;
  c2.x1[2] = 0;
}

void NPR3D1::distributeParam()
{
  if (param.size() < 5){
    std::cout << "NPR3D1 wrong param size. \n";
    return;
  }
  core.r = param[0];
  core.x1[0] = param[1];
  core.x1[1] = core.x1[0];
  core.x1[2] = core.x1[0];

  c1.r = param[3];
  c1.x0[0] = core.x1[0] + 0.5 * c1.r;
  c1.x0[1] = core.x1[1] - 0.5 * c1.r;
  c1.x0[2] = core.x1[2];
  c1.x1[0] = param[2] + 0.5 * c1.r;
  c1.x1[1] = param[2] - 0.5 * c1.r;

  c2.r = param[4];
  c2.x0[0] = c1.x1[0] - 0.5 *c1.r;
  c2.x0[1] = c1.x1[1] + 0.5 *  c1.r;
  c2.x0[2] = c1.x1[2];
}

void NPR3D1::extractParam()
{
  param.resize(5);
  //core.r, core.x1[0], c1.x1[0], c1.r, c2.r
  param[0] = core.r;
  param[1] = core.x1[0];
  param[2] = c1.x1[0];
  param[3] = c1.r;
  param[4] = c2.r;
}

void NPR3D1::eval(const std::vector<double> & p)
{
  param = p;
  distributeParam();
  std::fill(s.begin(), s.end(), 0.0);
  drawCylinder(core, s, gridSize, 1.0);
  drawCylinder(c1, s, gridSize, 1.0);
  drawCylinder(c2, s, gridSize, 1.0);
  s = mirrorCubicStructure(s, gridSize);

}


NPR3D2::NPR3D2()
{
  core.r = 0.1;
  core.x0[0] = 0.5;
  core.x0[1] = 0.5;
  core.x0[2] = 0.5;
  core.x1[0] = 0.5;
  core.x1[1] = 0.5;
  core.x1[2] = 0.1;

  beamlen = 0.2;
  beam1.r = 0.03;
  beam1.x0[0] = core.x1[0];
  beam1.x0[1] = core.x1[1] - 0.5 * beam1.r;
  beam1.x0[2] = core.x1[2];
  beam1.x1[0] = beam1.x0[0] - beamlen;
  beam1.x1[1] = beam1.x0[1] - beamlen;
  beam1.x1[2] = beam1.x0[2];

  c1.r = 0.03;
  c1.x0[0] = beam1.x1[0];
  c1.x0[1] = beam1.x1[1];
  c1.x0[2] = beam1.x1[2];
  c1.x1[0] = beam1.x1[0];
  c1.x1[1] = beam1.x1[1];
  c1.x1[2] = 0;

  c2.r = 0.03;
  c2.x0[0] = c1.x1[0];
  c2.x0[1] = c1.x1[1];
  c2.x0[2] = c1.x1[2];
  c2.x1[0] = 0;
  c2.x1[1] = 0;
  c2.x1[2] = 0;
}

void NPR3D2::distributeParam()
{
  if (param.size() < 5){
    std::cout << "NPR3D1 wrong param size. \n";
    return;
  }

}

void NPR3D2::extractParam()
{
  param.resize(5);
  //core.r, core.x1[0], c1.x1[0], c1.r, c2.r

}

void NPR3D2::eval(const std::vector<double> & p)
{
  param = p;
  distributeParam();
  std::fill(s.begin(), s.end(), 0.0);
  drawCylinder(core, s, gridSize, 1.0);
  drawCylinder(beam1, s, gridSize, 1.0);
  drawCylinder(c1, s, gridSize, 1.0);
  drawCylinder(c2, s, gridSize, 1.0);
  s = mirrorCubicStructure(s, gridSize);

}

void GraphStruct::load(std::string filename)
{
  loadGraph(filename, g);
  r.resize(g.E.size(), 0.03);
  x.resize(g.V.size());
  for (size_t i = 0; i < x.size(); i++){
    x[i] = g.V[i];
  }
}

void GraphStruct::distributeParam()
{
  int dim = 3;
  if (param.size() < (r.size() + dim*x.size())){ std::cout << "Graph Struct param wrong size.\n"; }
  for (size_t i = 0; i < r.size(); i++){
    r[i] = param[i];
  }
  int offset = (int)r.size();
  for (size_t i = 0; i < x.size(); i++){
    for (int j = 0; j < dim; j++){
      x[i][j] = param[offset + dim * i + j];
    }
  }
}

void GraphStruct::extractParam()
{
  //stack radii vertex positions.
  int dim = 3;
  param.resize(r.size() + dim*x.size());
  for (size_t i = 0; i < r.size(); i++){
    param[i] = r[i];
  }
  int offset = (int)r.size();
  for (size_t i = 0; i < x.size(); i++){
    for (int j = 0; j < dim; j++){
      param[offset + dim * i + j] = x[i][j];
    }
  }
}

void GraphStruct::eval(const std::vector<double> & p)
{
  param = p;
  distributeParam();
  int dim = 3;

  ////draw each beam.
  std::fill(s.begin(), s.end(), 0.0);
  Cylinder3D2 c;
  for (int i = 0; i < (int)g.E.size(); i++){
    int e0 = g.E[i][0];
    int e1 = g.E[i][1];
    c.r[0] = r[i];
    c.r[1] = r[i];
    for (int j = 0; j < dim; j++){
      c.x0[j] = x[e0][j];
      c.x1[j] = x[e1][j];
    }
    drawCylinder2(c, s, gridSize, 1.0);
  }
  s = mirrorCubicStructure(s, gridSize);
}


std::vector<double>
mirrorCubicStructure(const std::vector<double> &s, const std::vector<int> & gridSize)
{
  std::vector<double> t(s.size());
  for (int i = 0; i < gridSize[0]; i++) {
    for (int j = 0; j < gridSize[1]; j++) {
      for (int k = 0; k < gridSize[2]; k++) {
        int si = i;
        int sj = j;
        int sk = k;
        int tmp;
        if (si >= gridSize[0] / 2) {
          si = gridSize[0] - i - 1;
        }
        if (sj >= gridSize[1] / 2) {
          sj = gridSize[1] - j - 1;
        }
        if (sk >= gridSize[2] / 2) {
          sk = gridSize[2] - k - 1;
        }
        if (si < sj) {
          tmp = si;
          si = sj;
          sj = tmp;
        }
        if (si < sk) {
          tmp = sk;
          sk = si;
          si = tmp;
        }
        if (sj < sk) {
          tmp = sk;
          sk = sj;
          sj = tmp;
        }
        int idx = gridToLinearIdx(i, j, k, gridSize);
        int sidx = gridToLinearIdx(si, sj, sk, gridSize);
        t[idx] = s[sidx];
      }
    }
  }
  return t;
}
