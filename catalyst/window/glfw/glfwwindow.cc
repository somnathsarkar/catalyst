#include <catalyst/window/glfw/glfwwindow.h>
#include <catalyst/dev/dev.h>

namespace catalyst {
GlfwWindow::GlfwWindow() {
  window_ = nullptr;
  surface_ = VK_NULL_HANDLE;
  input_manager_ = nullptr;
}
void GlfwWindow::StartUp(int& argc, char** argv) {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window_ = glfwCreateWindow(1024, 576, "Catata", nullptr, nullptr);
  glfwSetWindowUserPointer(window_, this);
  surface_ = VK_NULL_HANDLE;
  input_manager_ = new InputManager();
  glfwSetMouseButtonCallback(window_, MouseButtonCallback);
  glfwSetCursorPosCallback(window_, CursorPositionCallback);
  glfwSetKeyCallback(window_, KeyCallback);
}
void GlfwWindow::SetVkInstance(VkInstance& instance) { instance_ = &instance; }
std::pair<uint32_t, uint32_t> GlfwWindow::GetExtent() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(window_, &width, &height);
  return std::make_pair(static_cast<uint32_t>(width),
                        static_cast<uint32_t>(height));
}
void GlfwWindow::ShutDown() {
  vkDestroySurfaceKHR(*instance_, surface_, nullptr);
  surface_ = VK_NULL_HANDLE;
  glfwDestroyWindow(window_);
  window_ = nullptr;
}
VkSurfaceKHR GlfwWindow::GetVkSurface() {
  if (surface_ == VK_NULL_HANDLE) {
    VkResult create_result = glfwCreateWindowSurface(*instance_, window_, nullptr, &surface_);
    ASSERT(create_result == VK_SUCCESS, "Failed to create window surface!");
  }
  return surface_;
}
bool GlfwWindow::IsOpen() { return !glfwWindowShouldClose(window_); }
void GlfwWindow::Update() {
  input_manager_->BeginUpdate();
  glfwPollEvents();
  input_manager_->EndUpdate();
}
void GlfwWindow::CaptureCursor() {
  glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}
void GlfwWindow::ReleaseCursor() {
  glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}
void GlfwWindow::LoadScene(Scene* scene) {}
void GlfwWindow::CursorPositionCallback(GLFWwindow* window, double xpos,
                                        double ypos) {
  GlfwWindow* glfw_window =
      reinterpret_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
  glm::vec2 new_mouse_pos = {static_cast<float>(xpos),
                             static_cast<float>(ypos)};
  glfw_window->input_manager_->GenerateMouseMoveEvent(new_mouse_pos);
}
void GlfwWindow::MouseButtonCallback(GLFWwindow* window, int button, int action,
                                     int mods) {
  std::map<int, MouseButton> glfw_mouse_map = {
      {GLFW_MOUSE_BUTTON_LEFT, MouseButton::kLeft}, {GLFW_MOUSE_BUTTON_RIGHT, MouseButton::kRight},
  };
  std::map<int, InputEventType> glfw_mouse_input_map = {
      {GLFW_PRESS, InputEventType::kDown},
      {GLFW_RELEASE, InputEventType::kUp},
  };
  GlfwWindow* glfw_window =
      reinterpret_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
  if (glfw_mouse_map.find(button) != glfw_mouse_map.end() &&
      glfw_mouse_input_map.find(action) != glfw_mouse_input_map.end())
    glfw_window->input_manager_->GenerateMouseButtonEvent(
        glfw_mouse_input_map[action], glfw_mouse_map[button]);
}
void GlfwWindow::KeyCallback(GLFWwindow* window, int key, int scancode,
                             int action, int mode) {
  std::map<int, Key> glfw_key_map = {
      {GLFW_KEY_W, Key::kW},
      {GLFW_KEY_S, Key::kS},
      {GLFW_KEY_A, Key::kA},
      {GLFW_KEY_D, Key::kD},
  };
  std::map<int, InputEventType> glfw_key_input_map = {
      {GLFW_PRESS, InputEventType::kDown},
      {GLFW_RELEASE, InputEventType::kUp},
  };
  GlfwWindow* glfw_window =
      reinterpret_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
  if (glfw_key_map.find(key) != glfw_key_map.end() &&
      glfw_key_input_map.find(action) != glfw_key_input_map.end())
    glfw_window->input_manager_->GenerateKeyboardEvent(
        glfw_key_input_map[action], glfw_key_map[key]);
}
}