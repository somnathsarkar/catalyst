#include <editor/window/editorwindow.h>

#include <QVulkanInstance>
#include <QFile>

#include <catalyst/dev/dev.h>
#include <editor/window/qtwindow.h>
#include <editor/window/qtviewportwindow.h>

namespace editor {
EditorWindow::EditorWindow() : scene_(nullptr){}
void EditorWindow::StartUp(int& argc, char** argv) {
  qapp_ = new QApplication(argc, argv);
  QFile style_file("../assets/theme/style.qss");
  ASSERT(style_file.exists(), "Could not load style file!");
  style_file.open(QFile::ReadOnly | QFile::Text);
  QTextStream style_stream(&style_file);
  qapp_->setStyleSheet(style_stream.readAll());
  window_ = new QtWindow(this);
  window_->show();
  input_manager_ = new catalyst::InputManager();
}
void EditorWindow::ShutDown() {
  delete window_;
  delete qapp_;
}
VkSurfaceKHR EditorWindow::GetVkSurface() {
  return QVulkanInstance::surfaceForWindow(window_->viewport_window_);
}
void EditorWindow::SetVkInstance(VkInstance& instance) {
  window_->instance_->setVkInstance(instance);
  window_->instance_->create();
  window_->viewport_window_->setVulkanInstance(window_->instance_);
}
std::pair<uint32_t, uint32_t> EditorWindow::GetExtent() {
  qreal pixel_ratio = window_->viewport_window_->devicePixelRatio();
  uint32_t width = window_->viewport_window_->width();
  uint32_t height = window_->viewport_window_->height();
  return std::make_pair(static_cast<uint32_t>(width * pixel_ratio),
                        static_cast<uint32_t>(height * pixel_ratio));
}
bool EditorWindow::IsOpen() { return !(window_->window_should_close); }
void EditorWindow::Update() {
  input_manager_->BeginUpdate();
  qapp_->processEvents();
  input_manager_->EndUpdate();
}
void EditorWindow::CaptureCursor() {
  window_->viewport_window_->CaptureCursor();
}
void EditorWindow::ReleaseCursor() {
  window_->viewport_window_->ReleaseCursor();
}
void EditorWindow::LoadScene(catalyst::Scene* scene) {
  scene_ = scene;
  window_->LoadScene();
}
}  // namespace editor