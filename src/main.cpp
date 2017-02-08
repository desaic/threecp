#include <nanogui/nanogui.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "AddPillar.hpp"
#include "ConfigFile.hpp"
#include "Element.hpp"
#include "ElementRegGrid.hpp"
#include "ElementMeshUtil.hpp"
#include "GraphUtil.hpp"
#include "linmath.h"
#include "gui.hpp"
#include "ParametricStructure3D.hpp"
#include "TrigMesh.hpp"
#include "VoxelIO.hpp"

void loadCuboids(std::string filename, std::vector<Cuboid> & cuboids);

void readVoxGraph(std::string filename, std::vector<int> & verts,
  std::vector<std::vector<int > > & edges)
{
  int nV = 0, nE = 0;
  FileUtilIn in (filename);

}

static void error_callback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void applyModifiers(const ConfigFile & conf, std::vector<double> & s, 
  std::vector<int> & gridSize)
{
  bool flip = false;
  bool mirror = false;
  bool repeat = false;
  bool cutTet = false;
  bool makePillar = false;
  conf.getBool("makePillar", makePillar);
  conf.getBool("mirror", mirror);
  conf.getBool("flip", flip);
  conf.getBool("repeat", repeat);
  conf.getBool("cutTet", cutTet);

  if (flip) {
    for (size_t j = 0; j < s.size(); j++) {
      if (s[j] > 0.5) {
        s[j] = 0;
      }
      else {
        s[j] = 1;
      }
    }
  }

  Cuboid cuboid;
  cuboid.x0[0] = 1;
  cuboid.x0[1] = 1;
  cuboid.x0[2] = 0.6;
  cuboid.x1[0] = 0.7;
  cuboid.x1[1] = 0.7;
  cuboid.x1[2] = 0.0;
  cuboid.r[0] = 0.2;
  cuboid.r[1] = 0.02;
  cuboid.theta = -0.78;
  //drawCuboid(cuboid, s, gridSize, 1);

  if (mirror) {
    s = mirrorOrthoStructure(s, gridSize);
  }
  if (repeat) {
    s = repeatStructure(s, gridSize, 2,2,1);
  }
  if (makePillar) {
    std::vector<double> support = addPillar(s, gridSize);
    s = support;
  }
  if (cutTet) {
    getCubicTet(s, gridSize);
  }
}

void readRenderConfig(const ConfigFile & conf, Render * render)
{
  std::vector<std::string> trigfiles = conf.getStringVector("trigmeshes");
  for (size_t i = 0; i < trigfiles.size(); i++) {
    FileUtilIn in;
    in.open(trigfiles[i]);
    if (!in.good()) {
      continue;
    }
    TrigMesh * tm = new TrigMesh();
    tm->load(in.in);
    render->trigs.push_back(tm);
    in.close();
  }
 
  bool saveObj = false;
  bool toGraph = false;
  bool convertSkel = false;
  bool saveTxt = false;

  conf.getBool("convertSkel", convertSkel);
  conf.getBool("saveObj", saveObj);
  conf.getBool("toGraph", toGraph);
  conf.getBool("saveTxt", saveTxt);

  if (convertSkel) {
    int inputres = 0;
    std::vector<double> sk;
    std::string skelFile = conf.dir + conf.getString("skelFile");
    loadArr3dTxt(skelFile, sk, inputres);
    std::vector<int> skelSize(3, inputres);
    saveBinaryStructure("tmp.bin", sk, skelSize);
  }
  Graph G;
  std::vector<std::string> voxFiles = conf.getStringVector("voxfiles");
  std::cout << "vox file " << voxFiles.size() << "\n";
  std::vector<int> gridSize;
  std::vector<double> s_all;
  ElementRegGrid * grid = new ElementRegGrid();
  for (size_t i = 0; i < voxFiles.size(); i++) {
    FileUtilIn in;
    in.open(voxFiles[i]);
    if (!in.good()) {
      continue;
    }
    std::vector<double> s;

    std::cout << "vox file " << voxFiles[i] << "\n";
    loadBinaryStructure(voxFiles[i], s, gridSize);
    //loadBinDouble(voxFiles[i], s, gridSize);

    applyModifiers(conf, s, gridSize);

    if (saveTxt && (i == 0) ) {
      FileUtilOut out("structure.txt");
      printIntStructure(s.data(), gridSize, out.out);
      out.close();
    }

    if (toGraph && (i == 0)) {
      float eps = 0.12f;
      voxToGraph(s, gridSize, G);
      //contractVertDegree2(G, eps);
      mergeCloseVerts(G, eps);
      contractPath(G, 0.1);
      saveGraph("skelGraph.txt", G);
    }
    
    std::cout << "Grid size " << gridSize[0] << " " << gridSize[1] << " " << gridSize[2] << ".\n";
    in.close();

    if (i == 0) {
      s_all.resize(s.size(), 0);
    }
    for (int j = 0; j < (int)s.size(); j++) {
      s_all[j] += (1+i) * s[j];
    }
  }
  
  if (voxFiles.size() > 0) {
    assignGridMat(s_all, gridSize, grid);
    render->meshes.push_back(grid);
    render->updateGrid(s_all, gridSize);
    if (saveObj) {
      TrigMesh tm;
      hexToTrigMesh(grid, &tm);
      std::string outfile = voxFiles[0] + ".obj";
      tm.save_obj(outfile.c_str());
    }
  }
  std::string graphFile = conf.getString("graph");

  if (graphFile.size() > 0) {
    loadGraph(graphFile, render->g);
    bool mirrorgraph = false;
    conf.getBool("mirrorgraph", mirrorgraph);
    if (mirrorgraph) {
      mirrorGraphCubic(render->g);
    }

    bool sep = false;
    conf.getBool("sep", sep);
    if (sep) {
      separateEdges(render->g);
      saveGraph("sepGraph.txt", render->g);
    }
  }
  if (conf.hasOpt("templateParam")) {
    std::string tpFile = conf.getString("templateParam");
    loadCuboids(tpFile, render->cuboids);
  }
}

int main(int argc, char * argv[])
{
  ViewerGUI * gui = new ViewerGUI();
  Render * render = new Render();
  gui->render = render;
  GLFWwindow* window;
  glfwSetErrorCallback(error_callback);
  if (!glfwInit()){ exit(EXIT_FAILURE); }
    
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_DEPTH_BITS, 32);
  window = glfwCreateWindow(800, 800, "Simple example", NULL, NULL);
  if (!window)
  {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glfwSetKeyCallback(window, key_callback);
  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  glfwSwapInterval(1);
  // NOTE: OpenGL error checks have been omitted for brevity
  render->window = window;
  gui->setWindow(window);
  gui->init();
  gui->screen->childAt(0)->setPosition(Eigen::Vector2i(10, 10));
  if (argc < 2) {
    std::cout << "Usage: view configfile\n";
    return -1;
  }
  std::string conffile(argv[1]);
  ConfigFile conf;
  int status = conf.load(conffile);
  if (status<0) {
    std::cout << "Cannot load " << conffile << "\n";
    return -1;
  }
  readRenderConfig(conf, render);
  render->loadShader("glsl/vs.txt", "glsl/fs.txt");
  render->init();
  double t0, t1;
  t0 = glfwGetTime();
  while (!glfwWindowShouldClose(window))
  {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glClearColor(1, 1, 1, 1.0);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    gui->drawContent();
    gui->screen->drawWidgets();
    glfwSwapBuffers(window);
    
    t1 = glfwGetTime();
    double dt = t1 - t0;
    t0 = t1;
    gui->render->moveCamera(dt);

    glfwPollEvents();
  }
  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}

void loadCuboids(std::string filename, std::vector<Cuboid> & cuboids)
{
  //list of vertex positions followed by list of beam parameters.
  //each beam has two vertices. There are 6N numbers for N beam coordinates and 3N numbers for beam sizing
  //and orientation.
  FileUtilIn in(filename);
  if (!in.good()) {
    return;
  }
  std::vector<std::vector<double> > params;
  in.readArr2d(params);
  in.close();
  if (params.size() == 0) {
    return;
  }
  int pidx = params.size() - 1;
  int nParam = (int)params[pidx].size();
  int dim = 3;
  int paramPerCube = 9;
  cuboids.resize(nParam / paramPerCube);

  //read positions
  for (size_t i = 0; i < cuboids.size(); i++) {
    for (int j = 0; j < dim; j++) {
      int idx = 6 * i + j + 3;
      if (idx >= (int)params[pidx].size()) {
        break;
      }
      //scale graph to 2x.
      //graph param is scaled back to [0 0.5] in input.
      cuboids[i].x0[j] = 2 * params[pidx][6 * i + j];
      cuboids[i].x1[j] = 2 * params[pidx][6 * i + j + 3];
    }
  }
  //read sizes
  for (size_t i = 0; i < cuboids.size(); i++) {
    int idx = 6 * cuboids.size() + 3 * i + 2;
    if (idx >= (int)params[pidx].size()) {
      break;
    }
    cuboids[i].r[0] = 2 * params[pidx][6 * cuboids.size() + 3 * i];
    cuboids[i].r[1] = 2 * params[pidx][6 * cuboids.size() + 3 * i + 1];
    cuboids[i].theta = params[pidx][6 * cuboids.size() + 3 * i + 2];
  }
}
