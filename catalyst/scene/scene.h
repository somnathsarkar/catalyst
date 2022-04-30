#pragma once
#include <vector>
#include <map>
#include <string>

#include <catalyst/scene/debugdrawobject.h>
#include <catalyst/scene/sceneobject.h>

namespace catalyst {
struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
};
struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};
enum class CameraType : uint32_t {
  kPerspective = 0,
  kOrthographic = 1,
};
class Camera {
 public:
  CameraType type_;
  float fovx_;

  Camera();
  glm::mat4 GetViewToClipTransform(uint32_t screen_width,
                                    uint32_t screen_height) const;

 private:
  glm::mat4 GetPerspectiveTransform(uint32_t screen_width,
                                    uint32_t screen_height) const;
  glm::mat4 GetOrthographicTransform(uint32_t screen_width,
                                     uint32_t screen_height) const;
};
enum class PrimitiveType {
  kEmpty = 0,
  kCube = 1,
  kTeapot = 2,
  kBunny = 3,
};
const std::string kPrimitiveNames[] = {"Empty", "Cube", "Teapot", "Bunny"};
class Scene {
 public:
  static const uint32_t kMaxDirectionalLights = 10;

  SceneObject* root_;
  std::vector<Mesh> meshes_;
  std::vector<uint32_t> offsets_;
  std::vector<Camera> cameras_;
  std::vector<DebugDrawObject*> debugdraw_objects_;

  Scene();
  ~Scene();

  MeshObject* AddPrimitive(SceneObject* parent, PrimitiveType type);
  CameraObject* AddCamera(SceneObject* parent, CameraType type);
  DirectionalLightObject* AddDirectionalLight(
      SceneObject* parent, glm::vec3 color = glm::vec3(1.0f));

  SceneObject* GetObjectByName(const std::string& name) const;

  glm::mat4 GetParentTransform(const SceneObject* scene_object) const;
  Aabb ComputeAabb(const std::vector<const SceneObject*> scene_objects) const;

  // Uncopyable
  Scene(const Scene& a) = delete;
  const Scene& operator=(const Scene& a) = delete;

 private:
  std::map<std::string,SceneObject*> name_map_;
  inline bool CheckNameExists(const std::string& s);
  std::string GetAvailableName(const std::string& prefix);
  void CreatePrimitives();
  Aabb ComputeAabb(const SceneObject* scene_object) const;
  void AabbDfs(const SceneObject* focus, Aabb& aabb,
               glm::mat4 transform = glm::mat4(1.0f)) const;
};
}  // namespace catalyst