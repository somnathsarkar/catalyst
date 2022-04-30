#pragma once
#include <QWidget>
#include <QWindow>
#include <QVulkanInstance>
#include <QGridLayout>
#include <QCloseEvent>
#include <QTreeView>

#include <editor/window/editorwindow.h>

namespace editor {
class EditorWindow::QtWindow : public QWidget {
 public:
  class QtViewportWindow;
  class QtSceneTree;
  class QtPropertiesPanel;
  bool selection_updated_;
  std::vector<const catalyst::SceneObject*> selection_;
  explicit QtWindow(EditorWindow* editor);
  ~QtWindow();
  void LoadScene();
  void closeEvent(QCloseEvent* close_event) override;
  QtViewportWindow* viewport_window_;
  QWidget* viewport_widget_;
  QVulkanInstance* instance_;
  QGridLayout* layout_;
  QtSceneTree* scene_tree_;
  QtPropertiesPanel* properties_panel_;
  bool window_should_close;

 private:
  EditorWindow* editor_;
};
}  // namespace editor