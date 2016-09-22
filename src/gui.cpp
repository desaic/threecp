#include "gui.hpp"
#include <nanogui\window.h>
#include <iostream>

using namespace nanogui;

//not thread safe. only works with 1 instance of viewergui.
ViewerGUI * viewergui;

void CursorPosCallback(GLFWwindow * window, double x, double y) 
{
  bool ret = viewergui->screen->cursorPosCallbackEvent(x, y);
  if (!ret) {
    //std::cout << "did not move on widgets.\n";
  }
}

void MouseButtonCallback(GLFWwindow *, int button, int action, int modifiers)
{
  bool ret = viewergui ->screen->mouseButtonCallbackEvent(button, action, modifiers);
  if (!ret) {
    //std::cout << "did not click on widgets.\n";
  }
}

void KeyCallback(GLFWwindow *, int key, int scancode, int action, int mods)
{
  bool ret = viewergui->screen->keyCallbackEvent(key, scancode, action, mods);
  if (!ret) {
    //std::cout << "no key on widgets.\n";
  }
}

void CharCallback(GLFWwindow *, unsigned int codepoint) {
  bool ret = viewergui->screen->charCallbackEvent(codepoint);
  if (!ret) {
    //std::cout << "no char on widgets.\n";
  }
}

void DropCallback(GLFWwindow *, int count, const char **filenames) {
  bool ret = viewergui->screen->dropCallbackEvent(count, filenames);
  if (!ret) {
    //std::cout << "no drop on widgets.\n";
  }
}

void ScrollCallback(GLFWwindow *, double x, double y) {
  bool ret = viewergui->screen->scrollCallbackEvent(x, y);
  if (!ret) {
    //std::cout << "no scroll on widgets.\n";
  }
}

void FramebufferSizeCallback(GLFWwindow *, int width, int height) {
  bool ret = viewergui->screen->resizeCallbackEvent(width, height);
  if (!ret) {
    //std::cout << "no resize on widgets.\n";
  }
}

void ViewerGUI::init()
{
  if (!window) {
    std::cout << "GUI init error. Need a window.\n";
  }
  // Create a nanogui screen and pass the glfw pointer to initialize
  screen = new Screen();
  screen->initialize(window, true);
  // Create nanogui gui
  bool enabled = true;
  FormHelper *gui = new FormHelper(screen);
  ref<Window> nanoguiWindow = gui->addWindow(Eigen::Vector2i(100, 100), "Form helper example");
  
  gui->addGroup("Basic types");
  gui->addVariable("bool", bvar)->setTooltip("Test tooltip.");
  gui->addVariable("string", strval);

  gui->addGroup("Validating fields");
  gui->addVariable("int", ivar)->setSpinnable(true);
  gui->addVariable("float", fvar)->setTooltip("Test.");
  gui->addVariable("double", dvar)->setSpinnable(true);

  gui->addGroup("Complex types");
  gui->addVariable("Enumeration", enumval, enabled)->setItems({ "Item 1", "Item 2", "Item 3" });
  gui->addVariable("Color", colval);

  gui->addGroup("Other widgets");
  gui->addButton("A button", []() { std::cout << "Button pressed." << std::endl; })->setTooltip("Testing a much longer tooltip, that will wrap around to new lines multiple times.");;

  screen->setVisible(true);
  screen->performLayout();
  nanoguiWindow->center();
  
  viewergui = this;

  glfwSetCursorPosCallback(window, CursorPosCallback);
  
  glfwSetMouseButtonCallback(window, MouseButtonCallback);

  glfwSetKeyCallback(window, KeyCallback);
    
  glfwSetCharCallback(window, CharCallback);
  
  glfwSetDropCallback(window, DropCallback  );

  glfwSetScrollCallback(window, ScrollCallback);

  glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

}

