#include "gui.hpp"
#include "Render.hpp"
#include "ElementMesh.hpp"
#include <nanogui\window.h>
#include <iostream>

using namespace nanogui;

//not thread safe. only works with 1 instance of viewergui.
ViewerGUI * viewergui;

void CursorPosCallback(GLFWwindow * window, double x, double y) 
{
  bool ret = viewergui->screen->cursorPosCallbackEvent(x, y);
  Render * r = viewergui->render;
  if (!ret) {
    //std::cout << "did not move on widgets.\n";
    if (r->captureMouse) {
      int width, height;
      glfwGetFramebufferSize(window, &width, &height);
      double dx = x - r->xpos0;
      double dy = -y + r->ypos0;
      r->xpos0 = x;
      r->ypos0 = y;
      //    std::cout<<dx<<" "<<dy<<"\n";
      //    glfwSetCursorPos(window,width/2, height/2);
      r->cam.angle_xz += (float)(dx * r->xRotSpeed);
      r->cam.angle_y -= (float)(dy * r->yRotSpeed);
      r->cam.update();
    }
  }
}

void MouseButtonCallback(GLFWwindow * window, int button, int action, int modifiers)
{
  bool ret = viewergui ->screen->mouseButtonCallbackEvent(button, action, modifiers);
  if (!ret) {
    //std::cout << "did not click on widgets.\n";
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
      if (action == GLFW_PRESS) {
        viewergui->render->captureMouse = true;
        double xpos, ypos;
        glfwGetCursorPos(window,&xpos, &ypos);
        viewergui->render->xpos0 = xpos;
        viewergui->render->ypos0 = ypos;
        //viewergui->render->pick(xpos, ypos);
      }
      else if (action == GLFW_RELEASE) {
        viewergui->render->captureMouse = false;
      }
      break;
    }
  }
}

void KeyCallback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
  Render * r = viewergui->render;
  bool ret = viewergui->screen->keyCallbackEvent(key, scancode, action, mods);
  if (!ret) {
    //std::cout << "no key on widgets.\n";
    if (action == GLFW_PRESS) {
      switch (key) {
      case GLFW_KEY_W:
        r->cam.keyhold[0] = true;
        break;
      case GLFW_KEY_S:
        r->cam.keyhold[1] = true;
        break;
      case GLFW_KEY_A:
        r->cam.keyhold[2] = true;
        break;
      case GLFW_KEY_D:
        r->cam.keyhold[3] = true;
        break;
      case GLFW_KEY_R:
        r->cam.keyhold[4] = true;
        break;
      case GLFW_KEY_F:
        r->cam.keyhold[5] = true;
        break;
      }
    }
    else if (action == GLFW_RELEASE) {
      switch (key) {
      case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GL_TRUE);
        break;
      case GLFW_KEY_W:
        r->cam.keyhold[0] = false;
        break;
      case GLFW_KEY_S:
        r->cam.keyhold[1] = false;
        break;
      case GLFW_KEY_A:
        r->cam.keyhold[2] = false;
        break;
      case GLFW_KEY_D:
        r->cam.keyhold[3] = false;
        break;
      case GLFW_KEY_R:
        r->cam.keyhold[4] = false;
        break;
      case GLFW_KEY_F:
        r->cam.keyhold[5] = false;
        break;
      }
    }
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

void ViewerGUI::ButtonCBOpen()
{
  //std::string filename = file_dialog(
  //{ { "bin", "Binary voxel data file" } }, false);
  //std::cout << "File dialog result: " << filename << std::endl;
  std::string indexFile = "C:\\Users\\desaic\\Desktop\\demo\\index.txt";
  std::ifstream idxIn(indexFile);
  int idx = 0;
  idxIn>> idx;
  idxIn.close();
  std::string prefix = "C:\\Users\\desaic\\Desktop\\demo\\64\\";
  std::string filename = prefix + std::to_string(idx) + ".bin";
  std::cout << "File name " << filename;
  Render * r = render;
  if (r->emEvent.size() == 0) {
    return;
  }
  if (filename.size() == 0) {
    return;
  }
  r->emEvent[0].eventType = Render::OPEN_FILE_EVENT;
  r->emEvent[0].filename = filename;
}

void ViewerGUI::ButtonCBSave()
{
  Render * r = render;
  if (r->meshes.size() > 0 && 
    (r->pickEvent.edges.size() > 0 
      ||r->pickEvent.verts.size()>0) ){
    r->pickEvent.saveGraph(r->meshes[0], &(r->grid) );
  }
}

void ViewerGUI::ButtonCBClear()
{
  Render * r = render;
  r->pickEvent.edges.clear();
  r->pickEvent.verts.clear();
  if (r->meshes.size() > 0) {
    for (size_t i = 0; i < r->meshes[0]->eLabel.size(); i++) {
      r->meshes[0]->eLabel[i] = 0;
    }
    r->copyEleBuffers(0);
    copyRayBuffers(r->pickEvent.buf, r->pickEvent, r->meshes[0]);
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
  ref<Window> nanoguiWindow = gui->addWindow(Eigen::Vector2i(100, 100), "File");

  gui->addGroup("File IO");
  std::function<void()> f_ButtonOpen = std::bind(&ViewerGUI::ButtonCBOpen, this);
  Button * b = gui->addButton("Open", f_ButtonOpen);
  b->setTooltip("Open a mesh file.");

  std::function<void()> f_ButtonSave = std::bind(&ViewerGUI::ButtonCBSave, this);
  Button * b1 = gui->addButton("Save", f_ButtonSave);
  b->setTooltip("Save current graph.");

  gui->addGroup("Graph");
  std::function<void()> f_ButtonClear = std::bind(&ViewerGUI::ButtonCBClear, this);
  Button * bc = gui->addButton("Clear", f_ButtonClear);
  b->setTooltip("Clear selection.");
  //gui->addVariable("bool", bvar)->setTooltip("Test tooltip.");
  //gui->addVariable("string", strval);

  gui->addGroup("View");
  IntBox<int> * sliceWidget = gui->addVariable("Slice", slice);
  sliceWidget->setSpinnable(true);
  sliceWidget->setMinMaxValues(0, 64);
  IntBox<int> * paramIdxWidget = gui->addVariable("param", paramIdx);
  paramIdxWidget->setTooltip("Parameter index");
  paramIdxWidget->setSpinnable(true);
  paramIdxWidget->setMinMaxValues(0, 100);
  //gui->addVariable("float", fvar)->setTooltip("Test.");
  //gui->addVariable("double", dvar)->setSpinnable(true);

  //gui->addGroup("Complex types");
  //gui->addVariable("Enumeration", enumval, enabled)->setItems({ "Item 1", "Item 2", "Item 3" });
  //gui->addVariable("Color", colval);

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

