#include <catalyst/scene/transform.h>

namespace catalyst {
Transform::Transform() : quaternion_(1.0f,0.0f,0.0f,0.0f), translation_(0.0f), scale_(1.0f){}
glm::quat Transform::GetQuaternion() const { return quaternion_; }
void Transform::SetQuaternion(glm::quat quaternion) {
  quaternion_ = quaternion;
}
glm::vec3 Transform::GetEulerAngles() const {
  return glm::degrees(glm::eulerAngles(quaternion_));
}
void Transform::SetEulerAngles(glm::vec3 euler_angles) {
  quaternion_ = glm::quat(glm::radians(euler_angles));
}
glm::vec3 Transform::GetTranslation() const { return translation_; }
void Transform::SetTranslation(glm::vec3 translation) {
  translation_ = translation;
}
glm::vec3 Transform::GetScale() const { return scale_; }
void Transform::SetScale(float uniform_scale) {
  scale_ = glm::vec3(uniform_scale);
}
void Transform::SetScale(glm::vec3 scale) { scale_ = scale; }
glm::mat4 Transform::GetTransformationMatrix() const {
  glm::mat4 result(1.0f);
  result = glm::scale(result, scale_);
  result = glm::toMat4(quaternion_) * result;
  result[3] = glm::vec4(translation_, 1.0f);
  return result;
}
glm::mat4 Transform::GetOrientationMatrix() const {
  return glm::toMat4(quaternion_);
}
}  // namespace catalyst