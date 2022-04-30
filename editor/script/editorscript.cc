#include <editor/script/editorscript.h>

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include <editor/window/editorwindow.h>
#include <editor/window/qtwindow.h>
#include <catalyst/dev/dev.h>

namespace editor {
void EditorScript::StartUp(catalyst::Window& window, catalyst::Scene& scene) {
  free_camera_enabled_ = false;
  catalyst::CameraObject* free_camera_obj =
      scene.AddCamera(scene.root_, catalyst::CameraType::kPerspective);
  free_camera_obj->external_ = true;
  glm::vec3 initial_position = glm::vec3(0.0f,-1.0f,0.0f);
  glm::vec3 initial_rotation = glm::vec3(0.0f);
  free_camera_ = new FreeCamera(*free_camera_obj,initial_position,initial_rotation);
}
void EditorScript::Update(catalyst::Window& window, catalyst::Scene& scene) {
  try {
    EditorWindow& editor_window = dynamic_cast<EditorWindow&>(window);
    catalyst::InputManager& input_manager = *(window.input_manager_);
    if (input_manager.mouse_state_[catalyst::MouseButton::kRight] ==
        catalyst::InputEventType::kDown) {
      if (!free_camera_enabled_) {
        window.CaptureCursor();
        free_camera_enabled_ = true;
      }
    } else {
      if (free_camera_enabled_) {
        window.ReleaseCursor();
        free_camera_enabled_ = false;
      }
    }
    if (free_camera_enabled_) {
      free_camera_->Update(input_manager);
    }
    // Selection Highlighting
    if (editor_window.window_->selection_updated_) {
      UpdateSelection(scene,editor_window.window_->selection_);
      editor_window.window_->selection_updated_ = false;
    }
  } catch (std::bad_cast e) {
    ASSERT(false, "Script attached to non-editor window!");
  }
}
void EditorScript::ShutDown() {

}
void EditorScript::UpdateSelection(
    catalyst::Scene& scene,
    std::vector<const catalyst::SceneObject*>& selection) {
  // Delete old selection
  for (catalyst::DebugDrawObject* debug_object : scene.debugdraw_objects_) {
    delete debug_object;
  }
  scene.debugdraw_objects_.clear();
  if (!selection.empty()) {
    catalyst::Aabb aabb = scene.ComputeAabb(selection);
    catalyst::DebugDrawAABB* debug_aabb = new catalyst::DebugDrawAABB(aabb);
    scene.debugdraw_objects_.push_back(debug_aabb);
  }
}
}