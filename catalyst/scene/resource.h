#pragma once
#include <string>
#include <cstdint>

#include <glm/glm.hpp>

#include <catalyst/scene/propertymanager.h>

namespace catalyst {
class Scene;
enum class ResourceType : uint32_t {
  kUndefined = 0,
  kMesh = 1,
  kMaterial = 2,
  kTexture = 3,
};
const std::string kResourceTypeNames[] = {"Undefined", "Mesh", "Material",
                                          "Texture"};
class Resource {
 public:
  std::string name_;
  ResourceType type_;
  PropertyManager property_manager_;
  Resource(Scene* scene, const std::string& name, const ResourceType type);

 protected:
  Scene* scene_;
};
struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
};
class Mesh : public Resource {
 public:
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  uint32_t material_id;
  Mesh(Scene* scene, const std::string& name);
};
class Material : public Resource {
 public:
  glm::vec3 albedo_;
  int albedo_texture_id_;

  float metallic_;
  int metallic_texture_id_;

  int normal_texture_id_;
  int height_texture_id_;

  float roughness_;
  int roughness_texture_id_;

  float reflectance_;
  Material(Scene* scene, const std::string& name);
};
struct Texture : public Resource {
 public:
  std::string path_;
  Texture(Scene* scene, const std::string& name);
};
}  // namespace catalyst