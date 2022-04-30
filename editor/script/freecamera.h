#pragma once
#include <catalyst/scene/sceneobject.h>
#include <catalyst/input/inputmanager.h>
#include <glm/glm.hpp>

namespace editor {
class FreeCamera {
 public:
  FreeCamera(catalyst::CameraObject& free_camera,
             const glm::vec3& initial_position,
             const glm::vec3& initial_rotation);
  void Update(catalyst::InputManager& input_manager);

 private:
  catalyst::CameraObject& camera_;
  glm::vec3 position_;
  glm::vec3 rotation_;
};
}