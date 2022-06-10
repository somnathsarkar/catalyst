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
  property_manager_.AddVec3Property("Translation", trans_getter, trans_setter,
                                    Vec3PropertyStyle::kSpinbox, -1000.0f,
                                    1000.0f);
  property_manager_.AddVec3Property("Orientation", rot_getter, rot_setter,
                                    Vec3PropertyStyle::kSpinbox, -90.0f, 90.0f);
  property_manager_.AddVec3Property("Scale", scale_getter, scale_setter,
                                    Vec3PropertyStyle::kSpinbox, 0.1f, 1000.0f);
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
DirectionalLightObject::DirectionalLightObject(
    Scene* scene, const std::string& name, glm::vec3 color, float cast_width,
    float cast_height, float cast_distance)
    : SceneObject(scene, name),
      color_(color),
      cast_width_(cast_width),
      cast_height_(cast_height),
      cast_distance_(cast_distance) {
  std::function<void(glm::vec3)> color_setter =
      [this](glm::vec3 new_value) -> void { this->color_ = new_value; };
  std::function<glm::vec3()> color_getter = [this]() -> glm::vec3 {
    return this->color_;
  };
  std::function<void(float)> width_setter = [this](float new_value) -> void {
    this->cast_width_ = new_value;
  };
  std::function<float()> width_getter = [this]() -> float {
    return this->cast_width_;
  };
  std::function<void(float)> height_setter = [this](float new_value) -> void {
    this->cast_height_ = new_value;
  };
  std::function<float()> height_getter = [this]() -> float {
    return this->cast_height_;
  };
  std::function<void(float)> dist_setter = [this](float new_value) -> void {
    this->cast_distance_ = new_value;
  };
  std::function<float()> dist_getter = [this]() -> float {
    return this->cast_distance_;
  };
  type_ = SceneObjectType::kDirectionalLight;
  property_manager_.AddVec3Property("Color", color_getter, color_setter,
                                    Vec3PropertyStyle::kColor, 0.0f, 1.0f);
  property_manager_.AddFloatProperty("Cast Width", width_getter, width_setter,
                                     0.5f, 100.0f);
  property_manager_.AddFloatProperty("Cast Height", height_getter,
                                     height_setter, 0.5f, 100.0f);
  property_manager_.AddFloatProperty("Cast Distance", dist_getter, dist_setter,
                                     0.5f, 100.0f);
}
glm::mat4 DirectionalLightObject::GetViewToClipTransform() const {
  const float near = 0.1f, far = cast_distance_;
  const float width = cast_width_, height = cast_height_;
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