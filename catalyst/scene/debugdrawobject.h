#pragma once
#include <catalyst/scene/sceneobject.h>

namespace catalyst {
enum class DebugDrawType : uint32_t{
  kWireframe = 0,
  kAABB = 1,
  kOOBB = 2,
  kBillboard = 3,
};
class DebugDrawObject {
 public:
  const DebugDrawType type_;
  virtual ~DebugDrawObject() = default;
  explicit DebugDrawObject(const DebugDrawType& type);
};
class DebugDrawWireframe : public DebugDrawObject {
 public:
  const SceneObject* scene_object_;
  explicit DebugDrawWireframe(const SceneObject* scene_object);
};
class DebugDrawAABB : public DebugDrawObject {
  public:
  Aabb aabb_;
   explicit DebugDrawAABB(const Aabb& aabb);
};
class DebugDrawBillboard : public DebugDrawObject {
 public:
  glm::vec3 position_;
  explicit DebugDrawBillboard(const glm::vec3& pos);
};
}  // namespace catalyst