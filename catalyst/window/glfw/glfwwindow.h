#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <catalyst/window/window.h>

namespace catalyst {
class GlfwWindow : public Window {
 public:
  GlfwWindow();
  void StartUp(int& argc, char** argv) override;
  void ShutDown() override;
  VkSurfaceKHR GetVkSurface() override;
  void SetVkInstance(VkInstance& instance) override;
  std::pair<uint32_t, uint32_t> GetExtent() override;
  bool IsOpen() override;
  void Update() override;
  void CaptureCursor() override;
  void ReleaseCursor() override;
  void LoadScene(Scene* scene) override;

private:
  GLFWwindow *window_;
  VkSurfaceKHR surface_;

  static void CursorPositionCallback(GLFWwindow* window, double xpos,
                                     double ypos);
  static void MouseButtonCallback(GLFWwindow* window, int button, int action,
                                  int mods);
  static void KeyCallback(GLFWwindow* window, int key, int scancode, int action,
                          int mods);
};
}  // namespace catalyst