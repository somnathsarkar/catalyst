#pragma once
#include <vector>
#include <string>

#include <glm/matrix.hpp>

#include <catalyst/scene/transform.h>
#include <catalyst/scene/propertymanager.h>

namespace catalyst {
class Scene;
enum class SceneObjectType : uint32_t {
  kDefault = 0,
  kMesh = 1,
  kCamera = 2,
  kDirectionalLight = 3,
};
class Aabb {
 public:
  float xmin, xmax;
  float ymin, ymax;
  float zmin, zmax;
  Aabb(const glm::vec3 point);
  ~Aabb() = default;
  void Extend(const Aabb& b);
  void Extend(const glm::vec3 b);
  static Aabb Merge(const Aabb& a, const Aabb& b);
};
class SceneObject {
 public:
  SceneObjectType type_;
  PropertyManager property_manager_;
  Transform transform_;
  SceneObject* parent_;
  std::vector<SceneObject*> children_;
  std::string name_;
  bool external_;

  SceneObject(Scene* scene, const std::string& name);
  ~SceneObject();
  // Uncopyable
  SceneObject(const SceneObject& a) = delete;
  const SceneObject& operator=(const SceneObject& a) = delete;
 private:
  Scene* scene_;
};
class MeshObject : public SceneObject {
 public:
  uint32_t mesh_id_;

  MeshObject(Scene* scene, const std::string& name, uint32_t mesh_id);
};
class CameraObject : public SceneObject {
 public:
  uint32_t camera_id_;

  CameraObject(Scene* scene, const std::string& name, uint32_t camera_id);
};
class DirectionalLightObject : public SceneObject {
 public:
  glm::vec3 color_;
  float cast_width_;
  float cast_height_;
  float cast_distance_;

  DirectionalLightObject(Scene* scene, const std::string& name,
                         glm::vec3 color = glm::vec3(1.0f),
                         float cast_width = 10.0f, float cast_height = 10.0f,
                         float cast_distance = 10.0f);
  glm::mat4 GetViewToClipTransform() const;
};
}