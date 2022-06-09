#include <catalyst/scene/resource.h>

#include <catalyst/scene/scene.h>

namespace catalyst {
Material::Material(Scene* scene, const std::string& name)
    : Resource(scene, name, ResourceType::kMaterial),
      albedo_(1.0f),
      albedo_texture_id_(-1),
      metallic_(0.0f),
      metallic_texture_id_(-1),
      normal_texture_id_(-1),
      roughness_(0.0f),
      roughness_texture_id_(-1),
      reflectance_(1.0f),
      ao_texture_id_(-1) {
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
  std::function<std::vector<std::string>()> texture_name_getter =
      [this]() -> std::vector<std::string> {
    std::vector<std::string> result;
    for (const Texture* tex : this->scene_->textures_) {
      result.push_back(tex->name_);
    }
    return result;
  };
  property_manager_.AddVec3Property("Albedo", color_getter, color_setter, 0.0f,
                                    1.0f);
  property_manager_.AddNamedIndexProperty(
      "Albedo Texture", Property::CreateIntegerGetter(&albedo_texture_id_),
      Property::CreateIntegerSetter(&albedo_texture_id_), texture_name_getter,
      NamedIndexPropertyStyle::kAllowNone);
  property_manager_.AddFloatProperty("Reflectance", reflectance_getter,
                                     reflectance_setter, 0.0f, 1.0f);
  property_manager_.AddFloatProperty("Metallic", metal_getter, metal_setter);
  property_manager_.AddNamedIndexProperty(
      "Metallic Texture", Property::CreateIntegerGetter(&metallic_texture_id_),
      Property::CreateIntegerSetter(&metallic_texture_id_), texture_name_getter,
      NamedIndexPropertyStyle::kAllowNone);
  property_manager_.AddFloatProperty("Roughness", rough_getter, rough_setter);
  property_manager_.AddNamedIndexProperty(
      "Roughness Texture",
      Property::CreateIntegerGetter(&roughness_texture_id_),
      Property::CreateIntegerSetter(&roughness_texture_id_),
      texture_name_getter, NamedIndexPropertyStyle::kAllowNone);
  property_manager_.AddNamedIndexProperty(
      "Normal Texture", Property::CreateIntegerGetter(&metallic_texture_id_),
      Property::CreateIntegerSetter(&metallic_texture_id_), texture_name_getter,
      NamedIndexPropertyStyle::kAllowNone);
  property_manager_.AddNamedIndexProperty(
      "AO Texture", Property::CreateIntegerGetter(&ao_texture_id_),
      Property::CreateIntegerSetter(&ao_texture_id_), texture_name_getter,
      NamedIndexPropertyStyle::kAllowNone);
}
Resource::Resource(Scene* scene, const std::string& name,
                   const ResourceType type)
    : name_(name), type_(type), property_manager_(), scene_(scene) {}
Mesh::Mesh(Scene* scene, const std::string& name)
    : Resource(scene, name, ResourceType::kMesh), material_id(0) {
  std::function<int()> mat_getter_ = [this]() -> int {
    return static_cast<int>(this->material_id);
  };
  std::function<void(int)> mat_setter_ = [this](int new_value) {
    this->material_id = static_cast<uint32_t>(new_value);
  };
  std::function<std::vector<std::string>()> name_getter_ =
      [this]() -> std::vector<std::string> {
    std::vector<std::string> name_list;
    for (Material* tex : this->scene_->materials_)
      name_list.push_back(tex->name_);
    return name_list;
  };
  property_manager_.AddNamedIndexProperty(
      "Material", mat_getter_, mat_setter_, name_getter_,
      NamedIndexPropertyStyle::kDisallowNone);
}
Texture::Texture(Scene* scene, const std::string& name)
    : Resource(scene, name, ResourceType::kTexture) {}
Cubemap::Cubemap(Scene* scene, const std::string& name)
    : Resource(scene, name, ResourceType::kCubemap) {}
Skybox::Skybox(Scene* scene, const std::string& name)
    : Resource(scene, name, ResourceType::kSkybox),
      specular_cubemap_id_(-1),
      diffuse_cubemap_id_(-1), specular_intensity_(0.1f), diffuse_intensity_(0.1f) {
  std::function<std::vector<std::string>()> cubemap_name_getter =
      [this]() -> std::vector<std::string> {
    std::vector<std::string> result;
    for (const Cubemap* cmap : this->scene_->cubemaps_) {
      result.push_back(cmap->name_);
    }
    return result;
  };
  property_manager_.AddNamedIndexProperty(
      "Specular Cubemap", Property::CreateIntegerGetter(&specular_cubemap_id_),
      Property::CreateIntegerSetter(&specular_cubemap_id_), cubemap_name_getter,
      NamedIndexPropertyStyle::kAllowNone);
  property_manager_.AddNamedIndexProperty(
      "Diffuse Cubemap", Property::CreateIntegerGetter(&diffuse_cubemap_id_),
      Property::CreateIntegerSetter(&diffuse_cubemap_id_), cubemap_name_getter,
      NamedIndexPropertyStyle::kAllowNone);
  property_manager_.AddFloatProperty(
      "Specular Intensity", Property::CreateFloatGetter(&specular_intensity_),
      Property::CreateFloatSetter(&specular_intensity_));
  property_manager_.AddFloatProperty(
      "Diffuse Intensity", Property::CreateFloatGetter(&diffuse_intensity_),
      Property::CreateFloatSetter(&diffuse_intensity_));
}
Settings::Settings(Scene* scene, const std::string& name)
    : Resource(scene, name, ResourceType::kSettings),
      exposure_adjustment_(2.0f),
      ssr_step_size_(0.05f),
      ssr_thickness_(0.1f),
      shadowmap_bias_(0.01f),
      shadowmap_kernel_size_(4),
      ssao_enabled_(true),
      ssr_enabled_(false) {
  property_manager_.AddFloatProperty(
      "Exposure Adjustment", Property::CreateFloatGetter(&exposure_adjustment_),
      Property::CreateFloatSetter(&exposure_adjustment_), 0.1f, 10.0f);
  property_manager_.AddFloatProperty(
      "SSR Step Size", Property::CreateFloatGetter(&ssr_step_size_),
      Property::CreateFloatSetter(&ssr_step_size_), 0.01f, 0.5f);
  property_manager_.AddFloatProperty(
      "SSR Thickness", Property::CreateFloatGetter(&ssr_thickness_),
      Property::CreateFloatSetter(&ssr_thickness_), 0.0001f, 1.0f);
  property_manager_.AddFloatProperty(
      "Shadowmap Bias", Property::CreateFloatGetter(&shadowmap_bias_),
      Property::CreateFloatSetter(&shadowmap_bias_), 0.0001f, 1.0f);
  property_manager_.AddIntegerProperty(
      "Shadowmap PCF Kernel Size",
      Property::CreateIntegerGetter(&shadowmap_kernel_size_),
      Property::CreateIntegerSetter(&shadowmap_kernel_size_), 1, 5);
  property_manager_.AddBooleanProperty(
      "SSAO Enabled", Property::CreateBooleanGetter(&ssao_enabled_),
      Property::CreateBooleanSetter(&ssao_enabled_));
  property_manager_.AddBooleanProperty(
      "SSR Enabled", Property::CreateBooleanGetter(&ssr_enabled_),
      Property::CreateBooleanSetter(&ssr_enabled_));
}
}  // namespace catalyst