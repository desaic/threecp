#include <nanogui/nanogui.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "linmath.h"
#include "gui.hpp"
#include "TrigMesh.hpp"

static void error_callback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}
int main(void)
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
  TrigMesh * tm = new TrigMesh();
  FileUtilIn in("data/bunny_sr.obj");
  tm->load(in.in);
  in.close();
  render->trigs.push_back(tm);
  render->loadShader("glsl/vs.txt", "glsl/fs.txt");
  render->init();

  while (!glfwWindowShouldClose(window))
  {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glClearColor(0.1, 0.1, 0.2, 1.0);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    gui->drawContent();
    gui->screen->drawWidgets();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}
