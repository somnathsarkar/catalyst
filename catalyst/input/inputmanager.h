#pragma once

#include <cstdint>
#include <map>

#include <glm/glm.hpp>

namespace catalyst {
enum class Key : uint32_t{
  kW = 0,
  kA = 1,
  kS = 2,
  kD = 3,
  kCount = 4,
};
enum class MouseButton : uint32_t {
  kLeft = 0,
  kRight = 1,
  kCount = 2,
};
enum class InputType : uint32_t {
  kKeyboard = 0,
  kMouse = 1,
  kCount = 2,
};
enum class InputEventType : uint32_t {
  kUp = 0,
  kDown = 1,
  kCount = 2,
};
class InputManager {
 public:
  glm::vec2 mouse_pos_;
  glm::vec2 delta_mouse_pos_;
  std::map<Key, InputEventType> key_state_;
  std::map<MouseButton, InputEventType> mouse_state_;
  InputManager();
  void BeginUpdate();
  void GenerateKeyboardEvent(InputEventType event_type, Key key);
  void GenerateMouseButtonEvent(InputEventType event_type, MouseButton key);
  void GenerateMouseMoveEvent(glm::vec2 new_mouse_pos_);
  void EndUpdate();

 private:
  glm::vec2 last_mouse_pos_;
  bool updating_;
  bool first_frame_;
};
}