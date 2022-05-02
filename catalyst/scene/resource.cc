#include <catalyst/scene/resource.h>

namespace catalyst {
Material::Material(const std::string& name)
    : Resource(name,ResourceType::kMaterial), albedo_(1.0f), reflectance_(1.0f), metallic_(0.0f), roughness_(0.0f) {
  std::function<glm::vec3()> color_getter = [this]() -> glm::vec3 {
    return this->albedo_;
  };
  std::function<void(glm::vec3)> color_setter =
      [this](glm::vec3 new_value) -> void { this->albedo_ = new_value; };
  std::function<float()> reflectance_getter = [this]() -> float {
    return this->reflectance_;
  };
  std::function<void(float)> reflectance_setter =
      [this](float new_value) -> void { this->reflectance_ = new_value; };
  std::function<float()> metal_getter = [this]() -> float {
    return this->metallic_;
  };
  std::function<void(float)> metal_setter = [this](float new_value) -> void {
    this->metallic_ = new_value;
  };
  std::function<float()> rough_getter = [this]() -> float {
    return this->roughness_;
  };
  std::function<void(float)> rough_setter = [this](float new_value) -> void {
    this->roughness_ = new_value;
  };
  property_manager_.AddVec3Property("Color", color_getter, color_setter, 0.0f,
                                    1.0f);
  property_manager_.AddFloatProperty("Reflectance", reflectance_getter,
                                     reflectance_setter, 0.0f, 1.0f);
  property_manager_.AddFloatProperty("Metallic", metal_getter, metal_setter);
  property_manager_.AddFloatProperty("Roughness", rough_getter, rough_setter);
}
Resource::Resource(const std::string& name, const ResourceType type)
    : name_(name), type_(type) {}
Mesh::Mesh(const std::string& name) : Resource(name, ResourceType::kMesh) {}

Texture::Texture(const std::string& name)
    : Resource(name, ResourceType::kTexture) {}
}  // namespace catalyst