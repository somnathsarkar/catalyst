#pragma once
#include <QWindow>

#include <editor/window/editorwindow.h>
#include <editor/window/qtwindow.h>

namespace editor {
class EditorWindow::QtWindow::QtViewportWindow final : public QWindow {
 public:
  QtViewportWindow(EditorWindow* editor);
  void CaptureCursor();
  void ReleaseCursor();
 protected:
  void mousePressEvent(QMouseEvent* mouse_event) override;
  void mouseReleaseEvent(QMouseEvent* mouse_event) override;
  void mouseMoveEvent(QMouseEvent* mouse_event) override;
  void keyPressEvent(QKeyEvent* key_event) override;
  void keyReleaseEvent(QKeyEvent* key_event) override;
 private:
  EditorWindow* editor_;
  bool cursor_captured_;
  QPoint capture_point_;
};
}