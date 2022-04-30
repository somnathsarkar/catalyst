#include <editor/window/qtviewportwindow.h>

#include <iostream>

#include <glm/glm.hpp>
#include <QMouseEvent>
#include <QCursor>

namespace editor {
EditorWindow::QtWindow::QtViewportWindow::QtViewportWindow(
    EditorWindow* editor) : editor_(editor), cursor_captured_(false){}
void EditorWindow::QtWindow::QtViewportWindow::CaptureCursor() {
  cursor_captured_ = true;
  //setCursor(Qt::BlankCursor);
}
void EditorWindow::QtWindow::QtViewportWindow::ReleaseCursor() {
  cursor_captured_ = false;
  //setCursor(Qt::ArrowCursor);
}
void EditorWindow::QtWindow::QtViewportWindow::mousePressEvent(
    QMouseEvent* mouse_event) {
  Qt::MouseButton button = mouse_event->button();
  std::map<Qt::MouseButton, catalyst::MouseButton> qt_mouse_map = {
      {Qt::LeftButton, catalyst::MouseButton::kLeft},
      {Qt::RightButton, catalyst::MouseButton::kRight}};
  if (qt_mouse_map.find(button) != qt_mouse_map.end())
    editor_->input_manager_->GenerateMouseButtonEvent(
        catalyst::InputEventType::kDown, qt_mouse_map[button]);
}
void EditorWindow::QtWindow::QtViewportWindow::mouseReleaseEvent(
    QMouseEvent* mouse_event) {
  Qt::MouseButton button = mouse_event->button();
  std::map<Qt::MouseButton, catalyst::MouseButton> qt_mouse_map = {
      {Qt::LeftButton, catalyst::MouseButton::kLeft},
      {Qt::RightButton, catalyst::MouseButton::kRight}};
  if (qt_mouse_map.find(button) != qt_mouse_map.end())
    editor_->input_manager_->GenerateMouseButtonEvent(
        catalyst::InputEventType::kUp, qt_mouse_map[button]);
}
void EditorWindow::QtWindow::QtViewportWindow::mouseMoveEvent(
    QMouseEvent* mouse_event) {

  glm::vec2 new_mouse_pos = {static_cast<float>(mouse_event->pos().x()),
                             static_cast<float>(mouse_event->pos().y())};
  editor_->input_manager_->GenerateMouseMoveEvent(new_mouse_pos);
}
void EditorWindow::QtWindow::QtViewportWindow::keyPressEvent(
  QKeyEvent* key_event) {
  Qt::Key key = static_cast<Qt::Key>(key_event->key());
  std::map<Qt::Key, catalyst::Key> qt_key_map = {
      {Qt::Key_W, catalyst::Key::kW},
      {Qt::Key_S, catalyst::Key::kS},
      {Qt::Key_A, catalyst::Key::kA},
      {Qt::Key_D, catalyst::Key::kD},
  };
  if (qt_key_map.find(key) != qt_key_map.end())
    editor_->input_manager_->GenerateKeyboardEvent(
        catalyst::InputEventType::kDown, qt_key_map[key]);
}
void EditorWindow::QtWindow::QtViewportWindow::keyReleaseEvent(
    QKeyEvent* key_event) {
  Qt::Key key = static_cast<Qt::Key>(key_event->key());
  std::map<Qt::Key, catalyst::Key> qt_key_map = {
      {Qt::Key_W, catalyst::Key::kW},
      {Qt::Key_S, catalyst::Key::kS},
      {Qt::Key_A, catalyst::Key::kA},
      {Qt::Key_D, catalyst::Key::kD},
  };
  if (qt_key_map.find(key) != qt_key_map.end())
    editor_->input_manager_->GenerateKeyboardEvent(
        catalyst::InputEventType::kUp, qt_key_map[key]);
}
}  // namespace editor