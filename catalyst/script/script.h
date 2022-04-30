#pragma once
#include <catalyst/window/window.h>
#include <catalyst/scene/scene.h>

namespace catalyst {
class Window;
class Scene;
class Script {
 public:
  Script();
  ~Script();
  virtual void StartUp(Window& window, Scene& scene) = 0;
  virtual void Update(Window& window, Scene& scene) = 0;
  virtual void ShutDown() = 0;

 private:
};
}