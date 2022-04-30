#include <editor/script/freecamera.h>

#include <glm/gtx/transform.hpp>

#include <catalyst/time/timemanager.h>

namespace editor {
FreeCamera::FreeCamera(catalyst::CameraObject& free_camera,
                       const glm::vec3& initial_position,
                       const glm::vec3& initial_rotation)
    : camera_(free_camera),
      position_(initial_position),
      rotation_(initial_rotation) {
  camera_.transform_.SetTranslation(initial_position);
}
void FreeCamera::Update(catalyst::InputManager& input_manager) {
  catalyst::TimeManager& time_manager = catalyst::TimeManager::Get();
  float delta_time = time_manager.game_clock_->delta_time_;
  float camera_speed = 2.0f;
  glm::vec3 local_movement(0.0f);
  if (input_manager.key_state_[catalyst::Key::kW] ==
      catalyst::InputEventType::kDown) {
    local_movement.y += 1;
  }
  if (input_manager.key_state_[catalyst::Key::kS] ==
      catalyst::InputEventType::kDown) {
    local_movement.y -= 1;
  }
  if (input_manager.key_state_[catalyst::Key::kA] ==
      catalyst::InputEventType::kDown) {
    local_movement.x -= 1;
  }
  if (input_manager.key_state_[catalyst::Key::kD] ==
      catalyst::InputEventType::kDown) {
    local_movement.x += 1;
  }
  local_movement *= delta_time * camera_speed;

  float camera_rot_speed = 2.0f;
  glm::vec3 delta_rotation = {-input_manager.delta_mouse_pos_.y, 0.0f,
                              -input_manager.delta_mouse_pos_.x};
  delta_rotation *= camera_rot_speed * delta_time;
  rotation_ += delta_rotation;

  glm::mat4 rot_z = glm::rotate(rotation_.z, glm::vec3(0.0f, 0.0f, 1.0f));
  glm::mat4 rot_x = glm::rotate(rotation_.x, glm::vec3(rot_z[0]));
  glm::mat4 orient = rot_x * rot_z;

  position_ += glm::vec3(orient[0]) * local_movement.x;
  position_ += glm::vec3(orient[1]) * local_movement.y;
  camera_.transform_.SetTranslation(position_);
  camera_.transform_.SetEulerAngles(glm::degrees(rotation_));
}
}  // namespace editor