#include <catalyst/scene/debugdrawobject.h>

namespace catalyst {
DebugDrawObject::DebugDrawObject(const DebugDrawType& type) : type_(type){};
DebugDrawWireframe::DebugDrawWireframe(const SceneObject* scene_object)
    : DebugDrawObject(DebugDrawType::kWireframe),scene_object_(scene_object){}
DebugDrawAABB::DebugDrawAABB(
    const Aabb& aabb)
    : DebugDrawObject(DebugDrawType::kAABB), aabb_(aabb) {
}
DebugDrawBillboard::DebugDrawBillboard(const glm::vec3& pos)
    : DebugDrawObject(DebugDrawType::kBillboard), position_(pos) {}
}  // namespace catalyst