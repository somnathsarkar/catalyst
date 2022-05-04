#include <catalyst/scene/sceneobject.h>

namespace catalyst {
SceneObject::SceneObject(Scene* scene, const std::string& name)
    : type_(SceneObjectType::kDefault),
      property_manager_(),
      transform_(),
      parent_(nullptr),
      children_(),
      name_(name),
      external_(false), scene_(scene) {
  std::function<glm::vec3()> trans_getter = [this]() -> glm::vec3 {
    return this->transform_.GetTranslation();
  };
  std::function<void(glm::vec3)> trans_setter = [this](glm::vec3 new_value) -> void {
    this->transform_.SetTranslation(new_value);
  };
  std::function<glm::vec3()> rot_getter = [this]() -> glm::vec3 {
    return this->transform_.GetEulerAngles();
  };
  std::function<void(glm::vec3)> rot_setter =
      [this](glm::vec3 new_value) -> void {
    this->transform_.SetEulerAngles(new_value);
  };
  std::function<glm::vec3()> scale_getter = [this]() -> glm::vec3 {
    return this->transform_.GetScale();
  };
  std::function<void(glm::vec3)> scale_setter =
      [this](glm::vec3 new_value) -> void {
    this->transform_.SetScale(new_value);
  };
  property_manager_.AddVec3Property("Translation", trans_getter, trans_setter, -1000.0f, 1000.0f);
  property_manager_.AddVec3Property("Orientation", rot_getter, rot_setter, -90.0f, 90.0f);
  property_manager_.AddVec3Property("Scale", scale_getter, scale_setter, 0.1f,
                                    1000.0f);
}
SceneObject::~SceneObject() {
  for (SceneObject* child:children_)
    delete child;
  children_.clear();
  parent_ = nullptr;
}
MeshObject::MeshObject(Scene* scene, const std::string& name, uint32_t mesh_id) : SceneObject(scene, name) {
  type_ = SceneObjectType::kMesh;
  mesh_id_ = mesh_id;
}
CameraObject::CameraObject(Scene* scene, const std::string& name,
                           uint32_t camera_id)
    : SceneObject(scene, name) {
  type_ = SceneObjectType::kCamera;
  camera_id_ = camera_id;
}
DirectionalLightObject::DirectionalLightObject(Scene* scene,
                                               const std::string& name,
                                               glm::vec3 color)
    : SceneObject(scene, name), color_(color) {
  std::function<void(glm::vec3)> color_setter =
      [this](glm::vec3 new_value) -> void { this->color_ = new_value; };
  std::function<glm::vec3()> color_getter = [this]() -> glm::vec3 {
    return this->color_;
  };
  type_ = SceneObjectType::kDirectionalLight;
  property_manager_.AddVec3Property("Color", color_getter, color_setter, 0.0f,
                                    1.0f);
}
glm::mat4 DirectionalLightObject::GetViewToClipTransform() {
  const float near = 0.1f, far = 50.0f;
  const float width = 10.0f, height = 10.0f;
  glm::mat4 proj_mat(0.0f);
  proj_mat[0][0] = 2.0f / width;
  proj_mat[1][1] = 1.0f / (far - near);
  proj_mat[3][1] = -near / (far - near);
  proj_mat[2][2] = 2.0f / height;
  proj_mat[3][3] = 1.0f;

  glm::mat4 clip_mat(1.0f);
  clip_mat[1][1] = clip_mat[2][2] = 0.0f;
  clip_mat[2][1] = -1.0f;
  clip_mat[1][2] = 1.0f;
  return clip_mat*proj_mat;
}
Aabb::Aabb(const glm::vec3 point)
    : xmin(point.x),
      xmax(point.x),
      ymin(point.y),
      ymax(point.y),
      zmin(point.z),
      zmax(point.z) {}
void Aabb::Extend(const Aabb& b) {
  xmin = std::min(xmin, b.xmin);
  xmax = std::max(xmax, b.xmax);
  ymin = std::min(ymin, b.ymin);
  ymax = std::max(ymax, b.ymax);
  zmin = std::min(zmin, b.zmin);
  zmax = std::max(zmax, b.zmax);
}
void Aabb::Extend(const glm::vec3 b) {
  xmin = std::min(xmin, b.x);
  xmax = std::max(xmax, b.x);
  ymin = std::min(ymin, b.y);
  ymax = std::max(ymax, b.y);
  zmin = std::min(zmin, b.z);
  zmax = std::max(zmax, b.z);
}
Aabb Aabb::Merge(const Aabb& a, const Aabb& b) {
  Aabb c = a;
  c.Extend(b);
  return c;
}
}  // namespace catalyst