#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include <catalyst/application/application.h>
#include <catalyst/time/timemanager.h>
#include <catalyst/window/glfw/glfwwindow.h>
#include <editor/window/editorwindow.h>
#include <editor/script/editorscript.h>

int main(int argc, char** argv) {
  catalyst::Scene scene;
  catalyst::Application app;
  editor::EditorWindow window;
  editor::EditorScript script;
  app.AssignWindow(&window);
  app.StartUp(argc,argv);
  app.LoadScene(&scene);
  app.LoadScript(&script);
  while (window.IsOpen()) {
    // Update
    app.Update();
  }
  app.ShutDown();
  return 0;
}