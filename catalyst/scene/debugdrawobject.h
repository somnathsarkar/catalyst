#include <catalyst/scene/sceneobject.h>

namespace catalyst {
enum class DebugDrawType : uint32_t{
  kWireframe = 0,
  kAABB = 1,
  kOOBB = 2,
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
}  // namespace catalyst