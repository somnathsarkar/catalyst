#pragma once
#include <QApplication>

#include <catalyst/window/window.h>

namespace editor {
class EditorWindow : public catalyst::Window {

 public:
  class QtWindow;
  QtWindow* window_;
  catalyst::Scene* scene_;
  EditorWindow();
  void StartUp(int& argc, char** argv) override;
  void ShutDown() override;
  VkSurfaceKHR GetVkSurface() override;
  void SetVkInstance(VkInstance& instance) override;
  std::pair<uint32_t, uint32_t> GetExtent() override;
  bool IsOpen() override;
  void Update() override;
  void CaptureCursor() override;
  void ReleaseCursor() override;
  void LoadScene(catalyst::Scene* scene) override;

 private:
  QApplication* qapp_;
};
}  // namespace editor