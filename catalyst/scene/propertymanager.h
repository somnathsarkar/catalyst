#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <functional>

#include <glm/glm.hpp>

namespace catalyst {
enum class PropertyType {
  kBoolean = 0,
  kInteger = 1,
  kFloat = 2,
  kString = 3,
  kVec3 = 4,
};
class Property {
  friend class PropertyManager;
 public:
  const std::string name_;
  const PropertyType type_;

 protected:
  Property(const std::string& property_name, const PropertyType& property_type);
};
class BooleanProperty : public Property {
  friend class PropertyManager;
 public:
  std::function<bool()> getter_;
  std::function<void(bool)> setter_;

 protected:
  BooleanProperty(const std::string& property_name,
                  std::function<bool()> getter,
                  std::function<void(bool)> setter);
};
class IntegerProperty : public Property {
  friend class PropertyManager;
 public:
  std::function<int()> getter_;
  std::function<void(int)> setter_;

 protected:
  IntegerProperty(const std::string& property_name, std::function<int()> getter,
                  std::function<void(int)> setter);
};
class FloatProperty : public Property {
  friend class PropertyManager;
 public:
  std::function<float()> getter_;
  std::function<void(float)> setter_;
  float min_value_;
  float max_value_;

 protected:
  FloatProperty(const std::string& property_name, std::function<float()> getter,
                std::function<void(float)> setter, float min_value = 0.0f,
                float max_value = 1.0f);
};
class StringProperty : public Property {
  friend class PropertyManager;
 public:
  std::function<std::string()> getter_;
  std::function<void(std::string)> setter_;

 protected:
  StringProperty(const std::string& property_name,
                 std::function<std::string()> getter,
                 std::function<void(std::string)> setter);
};
class Vec3Property : public Property {
  friend class PropertyManager;
 public:
  std::function<glm::vec3()> getter_;
  std::function<void(glm::vec3)> setter_;
  float min_value_;
  float max_value_;

 protected:
  Vec3Property(const std::string& property_name,
               std::function<glm::vec3()> getter,
               std::function<void(glm::vec3)> setter, float min_value = 0.0f,
               float max_value = 100.0f);
};
class PropertyManager {
 public:
  PropertyManager();
  ~PropertyManager();
  void AddBooleanProperty(const std::string& property_name,
                          std::function<bool()> getter,
                          std::function<void(bool)> setter);
  void AddFloatProperty(const std::string& property_name,
                        std::function<float()> getter,
                        std::function<void(float)> setter,
                        float min_value = 0.0f, float max_value = 1.0f);
  void AddVec3Property(const std::string& property_name,
                       std::function<glm::vec3()> getter,
                       std::function<void(glm::vec3)> setter,
                       float min_value = 0.0f, float max_value = 100.0f);
  uint32_t PropertyCount() const;
  Property* GetProperty(uint32_t property_index) const;

 private:
  std::vector<Property*> properties_;
};
}  // namespace catalyst