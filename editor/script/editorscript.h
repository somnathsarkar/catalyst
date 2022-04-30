#pragma once
#include <catalyst/script/script.h>

#include <editor/script/freecamera.h>

namespace editor {
class EditorScript : public catalyst::Script {
 public:
  void StartUp(catalyst::Window& window, catalyst::Scene& scene) override;
  void Update(catalyst::Window& window, catalyst::Scene& scene) override;
  void ShutDown() override;

 private:
  bool free_camera_enabled_;
  editor::FreeCamera* free_camera_;
  void UpdateSelection(catalyst::Scene& scene,
                       std::vector<const catalyst::SceneObject*>& selection);
};
}