#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace catalyst {
class Transform {
 public:
  Transform();
  glm::quat GetQuaternion() const;
  void SetQuaternion(glm::quat quaternion);
  glm::vec3 GetEulerAngles() const;
  void SetEulerAngles(glm::vec3 euler_angles);
  glm::vec3 GetTranslation() const;
  void SetTranslation(glm::vec3 translation);
  glm::vec3 GetScale() const;
  void SetScale(float uniform_scale);
  void SetScale(glm::vec3 scale);
  glm::mat4 GetTransformationMatrix() const;
  glm::mat4 GetOrientationMatrix() const;
 private:
  glm::quat quaternion_;
  glm::vec3 translation_;
  glm::vec3 scale_;
};
}