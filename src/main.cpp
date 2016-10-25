#include <nanogui/nanogui.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "ConfigFile.hpp"
#include "Element.hpp"
#include "ElementRegGrid.hpp"
#include "ElementMeshUtil.hpp"
#include "linmath.h"
#include "gui.hpp"
#include "TrigMesh.hpp"
#include "VoxelIO.hpp"

static void error_callback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
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

  std::vector<std::string> voxFiles = conf.getStringVector("voxfiles");
  for (size_t i = 0; i < voxFiles.size(); i++) {
    FileUtilIn in;
    in.open(voxFiles[i]);
    if (!in.good()) {
      continue;
    }
    std::vector<int> gridSize;
    std::vector<double> s;
    ElementRegGrid * grid = new ElementRegGrid();
    loadBinaryStructure(voxFiles[i], s, gridSize);
    //s = mirrorOrthoStructure(s, gridSize);
    //loadBinDouble(voxFiles[i], s, gridSize);
    assignGridMat(s, gridSize, grid);
    if (i == 0) {
      render->updateGrid(s, gridSize);
    }
    //TrigMesh tm;
    //hexToTrigMesh(grid, &tm);
    //std::string outfile = voxFiles[i] + ".obj";
    //tm.save_obj(outfile.c_str());

    render->meshes.push_back(grid);
    in.close();
  }



}

int main(int argc, char * argv[])
{
  ViewerGUI * gui = new ViewerGUI();
  Render * render = new Render();
  gui->render = render;
  GLFWwindow* window;
  glfwSetErrorCallback(error_callback);
  if (!glfwInit())
    exit(EXIT_FAILURE);
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
    glClearColor(0.9, 0.9, 0.9, 1.0);
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
