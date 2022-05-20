#pragma once
#include <QWidget>
#include <QWindow>
#include <QVulkanInstance>
#include <QGridLayout>
#include <QCloseEvent>
#include <QTreeView>

#include <editor/window/editorwindow.h>

namespace editor {
enum class SelectionType : uint32_t {
  kObject = 0,
  kResource = 1,
};
class EditorWindow::QtWindow : public QWidget {
 public:
  class QtViewportWindow;
  class QtSceneTree;
  class QtResourcePanel;
  class QtPropertiesPanel;
  SelectionType selection_type_;
  bool selection_updated_;
  std::vector<const catalyst::SceneObject*> object_selection_;
  std::vector<const catalyst::Resource*> resource_selection_;
  explicit QtWindow(EditorWindow* editor);
  ~QtWindow();
  void LoadScene();
  void closeEvent(QCloseEvent* close_event) override;
  QtViewportWindow* viewport_window_;
  QWidget* viewport_widget_;
  QVulkanInstance* instance_;
  QGridLayout* layout_;
  QTabWidget* tab_view_;
  QtSceneTree* scene_tree_;
  QtResourcePanel* resource_panel_;
  QtPropertiesPanel* properties_panel_;
  bool window_should_close;

 private:
  EditorWindow* editor_;
};
}  // namespace editor