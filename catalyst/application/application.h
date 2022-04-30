#pragma once
#include <catalyst/window/window.h>
#include <catalyst/dev/dev.h>
#include <catalyst/scene/scene.h>
#include <catalyst/script/script.h>

namespace catalyst {
class Window;
class Script;
class Scene;
class Application {
 class Renderer;

 public:
  Scene* scene_;
  Script* script_;
  Application();
  void StartUp(int argc, char** argv);
  void ShutDown();

  void AssignWindow(Window* window);
  void LoadScene(Scene* scene);
  void LoadScript(Script* script);
  void Update();

  // Uncopyable
  Application(const Application& a) = delete;
  const Application& operator=(const Application& a) = delete;

 private:
  int argc;
  char** argv;

  Window* main_window;
  Renderer* renderer;
};
}