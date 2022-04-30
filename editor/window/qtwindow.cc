#include <editor/window/qtwindow.h>

#include <editor/window/qtviewportwindow.h>
#include <editor/window/qtscenetree.h>
#include <editor/window/qtpropertiespanel.h>

namespace editor {
EditorWindow::QtWindow::QtWindow(EditorWindow* editor):editor_(editor),selection_updated_(false){
  uint32_t width = 1024, height = 576;
  viewport_window_ = new QtViewportWindow(editor);
  viewport_window_->resize(width, height);
  viewport_widget_ = QWidget::createWindowContainer(viewport_window_);
  viewport_widget_->resize(width, height);
  instance_ = new QVulkanInstance();
  viewport_window_->setSurfaceType(QSurface::SurfaceType::VulkanSurface);

  layout_ = new QGridLayout(this);
  layout_->setColumnMinimumWidth(0, width);
  layout_->setRowMinimumHeight(0, height / 2);
  layout_->setRowMinimumHeight(1, height / 2);
  layout_->setContentsMargins(0, 0, 0, 0);
  layout_->addWidget(viewport_widget_,0,0,2,1);

  scene_tree_ = new QtSceneTree(this);
  layout_->addWidget(scene_tree_, 0, 1);

  properties_panel_ = new QtPropertiesPanel(this);
  layout_->addWidget(properties_panel_, 1, 1);
  window_should_close = false;

  showMaximized();
}

EditorWindow::QtWindow::~QtWindow() {}

void EditorWindow::QtWindow::LoadScene() { scene_tree_->LoadScene(); }

void EditorWindow::QtWindow::closeEvent(QCloseEvent* close_event) {
  window_should_close = true;
  close_event->ignore();
}
}  // namespace editor