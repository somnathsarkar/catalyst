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
  kCubemap = 4,
  kSkybox = 5,
  kSettings = 6,
};
const std::string kResourceTypeNames[] = {"Undefined", "Mesh",    "Material",
                                          "Texture",   "Cubemap", "Skybox",
                                          "Settings"};
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
  glm::vec3 tangent;
  glm::vec3 bitangent;
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

  float roughness_;
  int roughness_texture_id_;

  float reflectance_;

  int ao_texture_id_;
  Material(Scene* scene, const std::string& name);
};
class Texture : public Resource {
 public:
  std::string path_;
  Texture(Scene* scene, const std::string& name);
};
class Cubemap : public Resource {
 public:
  std::string path_;
  Cubemap(Scene* scene, const std::string& name);
};
class Skybox : public Resource {
 public:
  int specular_cubemap_id_;
  int diffuse_cubemap_id_;
  float specular_intensity_;
  float diffuse_intensity_;
  Skybox(Scene* scene, const std::string& name);
};
class Settings : public Resource {
 public:
  float exposure_adjustment_;
  float ssr_step_size_;
  float ssr_thickness_;
  Settings(Scene* scene, const std::string& name);
};
}  // namespace catalyst