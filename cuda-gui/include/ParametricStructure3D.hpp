#ifndef PARAMETRIC_STRUCTURE_3D_HPP
#define PARAMETRIC_STRUCTURE_3D_HPP

#include <vector>
#include "Graph.hpp"

struct Cylinder3D
{
  float x0[3];
  float x1[3];
  float r;
  Cylinder3D():r(0.0f){
    for (int i = 0; i < 3; i++){
      x0[i] = 0.0f;
      x1[i] = 0.0f;
    }
  }
};

//cylinder with two radii at end points.
struct Cylinder3D2
{
  float x0[3];
  float x1[3];
  float r[2];
  Cylinder3D2(){
    for (int i = 0; i < 3; i++){
      x0[i] = 0.0f;
      x1[i] = 0.0f;
    }
    r[0] = 0.0f;
    r[1] = 0.0f;
  }
  ~Cylinder3D2(){}
};

struct Cuboid
{
  float x0[3];
  float x1[3];
  float r[2];
  float theta;
  bool withCaps;
  Cuboid(){
    for (int i = 0; i < 3; i++){
      x0[i] = 0;
      x1[i] = 0;
    }
    r[0] = 0;
    r[1] = 0;
    theta = 0;
    withCaps = 1;
  }
};

class ParametricStructure3D{
public:
  ParametricStructure3D(){
    gridSize.resize(3, 0);
  }

  ParametricStructure3D(int nx, int ny, int nz)
  {
    resize(nx, ny, nz);
  }
  
  void resize(int nx, int ny, int nz);

  virtual void eval(const std::vector<double> & p){
    param = p;
  }

  std::vector<double> s;
  std::vector<int> gridSize;
  //shape parameters.
  std::vector<double> param;

};

class ParametricNPR3D : public ParametricStructure3D{
public:
  ParametricNPR3D();

  //outer shell
  //diagonal  and horizontal.
  Cylinder3D c, c1;

  //inner core
  Cylinder3D core;

  //connections
  Cylinder3D con1, con2;

  //7 params. c.x0[0]  c.x1[0] c.r
  //core.r
  //con1.x1
  void distributeParam();

  //read variable parameters from geometric primitives
  void extractParam();

  void eval(const std::vector<double> & p);
};

class ParamShear3D : public ParametricStructure3D{
public:
  ParamShear3D(){
    gridSize.resize(3, 0);
    c.r = 0.03;
    c.x0[0] = 0.5 - c.r - 0.02;
    c.x0[1] = c.r + 0.02;
    c.x0[2] = c.r;
    c.x1[0] = 0.22;
    c.x1[1] = c.x1[0];
    c.x1[2] = c.r;
    
    c1.r = c.r;
    c1.x0[0] = c.x0[0];
    c1.x0[1] = c.x0[1];
    c1.x0[2] = c.x0[2];
    c1.x1[0] = c.x0[1];
    c1.x1[1] = c1.x1[0];
    c1.x1[2] = c1.x1[0];

    core.r = 0.07;
    core.x0[0] = 0.5;
    core.x0[1] = 0.5;
    core.x0[2] = 0.5;
    core.x1[0] = 0.15;
    core.x1[1] = 0.15;
    core.x1[2] = 0.15;

    con1.r = 0.02;
    con1.x0[0] = core.x1[0];
    con1.x0[1] = core.x1[1];
    con1.x0[2] = core.x1[2];
    con1.x1[0] = c.x1[0];
    con1.x1[1] = c.x1[1];
    con1.x1[2] = c.x1[2];
  }

  //outer shell
  Cylinder3D c, c1;

  //inner core
  Cylinder3D core;

  //connections
  Cylinder3D con1, con2;

  //distribute parameters into geometric primitives.
  //7 params. c.x0[0]  c.x1[0] c.r
  //core.r
  //con1.x1
  void distributeParam();

  //read variable parameters from geometric primitives
  void extractParam();

  void eval(const std::vector<double> & p);
};

class NPR3D1: public ParametricStructure3D
{
public:
  NPR3D1();

  //inner "X" core
  Cylinder3D core;
 
  //connector to face and corner
  Cylinder3D c1, c2;

  //core.r, core.x1[0], c1.x1[0], c1.r, c2.r
  void distributeParam();

  void extractParam();

  void eval(const std::vector<double> & p);
};

class NPR3D2 : public ParametricStructure3D
{
public:
  NPR3D2();

  //inner "+" core
  Cylinder3D core;

  //horizontal beam.
  Cylinder3D beam1;
  float beamlen;

  //connector to face and corner
  Cylinder3D c1, c2;

  //core.r, core.x1[0], c1.x1[0], c1.r, c2.r
  void distributeParam();

  void extractParam();

  void eval(const std::vector<double> & p);
};

class GraphStruct: public ParametricStructure3D
{
public:
  
  Graph g;
  
  //radii at verts.
  std::vector<float> r;
  
  //vertex positions possibly different from initial graph vertex positions.
  std::vector<Eigen::Vector3f> x;

  void load(std::string filename);

  void distributeParam();

  void extractParam();

  void eval(const std::vector<double> & p);
};

std::vector<double>
mirrorCubicStructure(const std::vector<double> &s, const std::vector<int> & gridSize);

void graphToCuboids(const Graph & g, std::vector<Cuboid> & cuboids);

void drawCylinder(Cylinder3D & cy, std::vector<double> & arr,
  const std::vector<int> & gridSize, double val);

void drawCylinder2(Cylinder3D2 & cy, std::vector<double> & arr,
  const std::vector<int> & gridSize, double val);

void drawCuboid(const Cuboid & c, std::vector<double> & arr,
  const std::vector<int> & gridSize, double val);
#endif