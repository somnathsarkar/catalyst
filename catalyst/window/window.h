#pragma once
#include <vulkan/vulkan.h>

#include <utility>

#include <catalyst/input/inputmanager.h>
#include <catalyst/scene/scene.h>

namespace catalyst {
class Window {
 public:
  VkInstance *instance_;
  InputManager *input_manager_;

  virtual void StartUp(int& argc, char **argv) = 0;
  virtual void ShutDown() = 0;
  virtual void SetVkInstance(VkInstance& instance) = 0;
  virtual VkSurfaceKHR GetVkSurface() = 0;
  virtual std::pair<uint32_t, uint32_t> GetExtent() = 0;
  virtual bool IsOpen() = 0;
  virtual void Update() = 0;
  virtual void CaptureCursor() = 0;
  virtual void ReleaseCursor() = 0;
  virtual void LoadScene(Scene* scene) = 0;
};
}