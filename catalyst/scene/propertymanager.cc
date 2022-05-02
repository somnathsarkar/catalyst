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
      case PropertyType::kFloat:
        delete static_cast<FloatProperty*>(prop);
        break;
      case PropertyType::kVec3:
        delete static_cast<Vec3Property*>(prop);
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
void PropertyManager::AddVec3Property(
    const std::string& property_name, std::function<glm::vec3()> getter,
    std::function<void(glm::vec3)> setter, float min_value, float max_value) {
  Vec3Property* prop =
      new Vec3Property(property_name, getter, setter, min_value, max_value);
  properties_.push_back(prop);
}
uint32_t PropertyManager::PropertyCount() const {
  return static_cast<uint32_t>(properties_.size());
}
Property* PropertyManager::GetProperty(
    uint32_t property_index) const {
  return properties_[property_index];
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
                                 std::function<void(int)> setter)
    : Property(property_name, PropertyType::kInteger),
      getter_(getter),
      setter_(setter) {}
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
                           float min_value, float max_value)
    : Property(property_name, PropertyType::kVec3),
      getter_(getter),
      setter_(setter),
      min_value_(min_value),
      max_value_(max_value)
{}
}  // namespace catalyst