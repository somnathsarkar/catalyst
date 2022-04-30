#pragma once
#include <vector>

#include <QWidget>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>

#include <catalyst/scene/propertymanager.h>
#include <editor/window/qtwindow.h>

namespace editor {
class EditorWindow::QtWindow::QtPropertiesPanel final : public QWidget{
 public:
  explicit QtPropertiesPanel(QtWindow* window_);
  void Update();
 private:
  class QtVec3Field : public QWidget {
   public:
    QtVec3Field(QtPropertiesPanel* property_panel, catalyst::Vec3Property* property);
    ~QtVec3Field();

   private:
    QtPropertiesPanel* property_panel_;
    catalyst::Vec3Property* property_;
    QHBoxLayout* layout_;
    QDoubleSpinBox* spinboxes_;
    glm::vec3 GetVec3() const;

   private slots:
    void ValueChanged(double v);
  };
  QtWindow* window_;
  QFormLayout* layout_;
  QLabel* property_label_;
};
}