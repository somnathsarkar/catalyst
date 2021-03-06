#include <catalyst/scene/propertymanager.h>

#include <catalyst/dev/dev.h>

namespace catalyst {
PropertyManager::PropertyManager() {}
PropertyManager::~PropertyManager() {
  for (uint32_t prop_i = 0; prop_i < properties_.size(); prop_i++) {
    Property* prop = properties_[prop_i];
    switch (prop->type_) {
      case PropertyType::kBoolean:
        delete static_cast<BooleanProperty*>(prop);
        break;
      case PropertyType::kInteger:
        delete static_cast<IntegerProperty*>(prop);
        break;
      case PropertyType::kFloat:
        delete static_cast<FloatProperty*>(prop);
        break;
      case PropertyType::kString:
        delete static_cast<StringProperty*>(prop);
        break;
      case PropertyType::kVec3:
        delete static_cast<Vec3Property*>(prop);
        break;
      case PropertyType::kNamedIndex:
        delete static_cast<NamedIndexProperty*>(prop);
        break;
      default:
        ASSERT(false, "Unhandled property type!");
        break;
    }
  }
  properties_.clear();
}
void PropertyManager::AddBooleanProperty(const std::string& property_name,
                                         std::function<bool()> getter,
                                         std::function<void(bool)> setter) {
  BooleanProperty* prop =
      new BooleanProperty(property_name, getter, setter);
  properties_.push_back(prop);
}
void PropertyManager::AddFloatProperty(const std::string& property_name,
                                       std::function<float()> getter,
                                       std::function<void(float)> setter,
                                       float min_value,
                                       float max_value) {
  FloatProperty* prop =
      new FloatProperty(property_name, getter, setter, min_value, max_value);
  properties_.push_back(prop);
}
void PropertyManager::AddStringProperty(
    const std::string& property_name, std::function<std::string()> getter,
    std::function<void(std::string)> setter) {
  StringProperty* prop =
      new StringProperty(property_name, getter, setter);
  properties_.push_back(prop);
}
void PropertyManager::AddIntegerProperty(const std::string& property_name,
                                         std::function<int()> getter,
                                         std::function<void(int)> setter,
                                         int min_value, int max_value) {
  IntegerProperty* prop =
      new IntegerProperty(property_name, getter, setter, min_value, max_value);
  properties_.push_back(prop);
}
void PropertyManager::AddNamedIndexProperty(
    const std::string& property_name, std::function<int()> getter,
    std::function<void(int)> setter,
    std::function<std::vector<std::string>()> name_getter, NamedIndexPropertyStyle style) {
  NamedIndexProperty* prop =
      new NamedIndexProperty(property_name, getter, setter, name_getter, style);
  properties_.push_back(prop);
}
void PropertyManager::AddVec3Property(
    const std::string& property_name, std::function<glm::vec3()> getter,
    std::function<void(glm::vec3)> setter, Vec3PropertyStyle style, float min_value, float max_value) {
  Vec3Property* prop =
      new Vec3Property(property_name, getter, setter, style, min_value, max_value);
  properties_.push_back(prop);
}
uint32_t PropertyManager::PropertyCount() const {
  return static_cast<uint32_t>(properties_.size());
}
Property* PropertyManager::GetProperty(
    uint32_t property_index) const {
  return properties_[property_index];
}
std::function<int(void)> Property::CreateIntegerGetter(int* int_field) {
  std::function<int(void)> getter = [int_field]() -> int { return *int_field; };
  return getter;
}
std::function<void(int)> Property::CreateIntegerSetter(int* int_field) {
  std::function<void(int)> setter = [int_field](int new_value) -> void { *int_field = new_value; };
  return setter;
}
std::function<float(void)> Property::CreateFloatGetter(float* field) {
  std::function<float(void)> getter = [field]() -> float {
    return *field;
  };
  return getter;
}
std::function<void(float)> Property::CreateFloatSetter(float* field) {
  std::function<void(float)> setter = [field](float new_value) -> void {
    *field = new_value;
  };
  return setter;
}
std::function<bool(void)> Property::CreateBooleanGetter(bool* field) {
  std::function<bool(void)> getter = [field]() -> bool { return *field; };
  return getter;
}
std::function<void(bool)> Property::CreateBooleanSetter(bool* field) {
  std::function<void(bool)> setter = [field](bool new_value) -> void {
    *field = new_value;
  };
  return setter;
}
Property::Property(const std::string& property_name,
                   const PropertyType& property_type) : name_(property_name),type_(property_type) {}
BooleanProperty::BooleanProperty(const std::string& property_name,
                                 std::function<bool()> getter,
                                 std::function<void(bool)> setter)
    : Property(property_name, PropertyType::kBoolean),
      getter_(getter),
      setter_(setter) {}
IntegerProperty::IntegerProperty(const std::string& property_name,
                                           std::function<int()> getter,
                                 std::function<void(int)> setter, int min_value, int max_value)
    : Property(property_name, PropertyType::kInteger),
      getter_(getter),
      setter_(setter),
      min_value_(min_value),
      max_value_(max_value) {}
FloatProperty::FloatProperty(const std::string& property_name,
                             std::function<float()> getter,
                             std::function<void(float)> setter, float min_value,
                             float max_value)
    : Property(property_name, PropertyType::kFloat),
      getter_(getter),
      setter_(setter),
      min_value_(min_value),
      max_value_(max_value) {}
StringProperty::StringProperty(
    const std::string& property_name, std::function<std::string()> getter,
    std::function<void(std::string)> setter)
    : Property(property_name, PropertyType::kString),
      getter_(getter),
      setter_(setter) {}
Vec3Property::Vec3Property(const std::string& property_name,
                           std::function<glm::vec3()> getter,
                           std::function<void(glm::vec3)> setter,
                           Vec3PropertyStyle style, float min_value,
                           float max_value)
    : Property(property_name, PropertyType::kVec3),
      getter_(getter),
      setter_(setter),
      style_(style),
      min_value_(min_value),
      max_value_(max_value) {}
NamedIndexProperty::NamedIndexProperty(
    const std::string& property_name, std::function<int()> getter,
    std::function<void(int)> setter,
    std::function<std::vector<std::string>()> name_getter,
    NamedIndexPropertyStyle style)
    : Property(property_name, PropertyType::kNamedIndex),
      getter_(getter),
      setter_(setter),
      name_getter_(name_getter),
      style_(style) {}
}  // namespace catalyst