#include <catalyst/input/inputmanager.h>

#include <catalyst/dev/dev.h>

namespace catalyst {
InputManager::InputManager()
    : mouse_pos_(0, 0),
      delta_mouse_pos_(0, 0),
      key_state_(),
      mouse_state_(),
      last_mouse_pos_(0, 0),
      updating_(false),
      first_frame_(true) {
  for (uint32_t key_i = 0; key_i < static_cast<uint32_t>(Key::kCount); key_i++)
    key_state_[static_cast<Key>(key_i)] = InputEventType::kUp;
  for (uint32_t mouse_i = 0; mouse_i < static_cast<uint32_t>(MouseButton::kCount); mouse_i++)
    mouse_state_[static_cast<MouseButton>(mouse_i)] = InputEventType::kUp;
}

void InputManager::BeginUpdate() {
  ASSERT(!updating_, "Attempting to start update while already updating!");
  updating_ = true;
  last_mouse_pos_ = mouse_pos_;
}
void InputManager::GenerateKeyboardEvent(InputEventType event_type, Key key) {
  ASSERT(updating_, "Attempting to process event without beginning update!");
  key_state_[key] = event_type;
}
void InputManager::GenerateMouseButtonEvent(InputEventType event_type,
                                 MouseButton mouse_button) {
  ASSERT(updating_, "Attempting to process event without beginning update!");
  mouse_state_[mouse_button] = event_type;
}
void InputManager::GenerateMouseMoveEvent(glm::vec2 new_mouse_pos) {
  ASSERT(updating_, "Attempting to process event without beginning update!");
  mouse_pos_ = new_mouse_pos;
}
void InputManager::EndUpdate() {
  ASSERT(updating_, "Did not begin update!");
  updating_ = false;
  if(!first_frame_)
    delta_mouse_pos_ = mouse_pos_ - last_mouse_pos_;
  else {
    delta_mouse_pos_ = glm::vec2(0.0f);
    first_frame_ = false;
  }
}
}