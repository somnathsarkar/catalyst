#pragma once
#include <vector>

#include <QWidget>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>

#include <catalyst/scene/propertymanager.h>
#include <editor/window/qtwindow.h>

namespace editor {
class EditorWindow::QtWindow::QtPropertiesPanel final : public QWidget{
  class QtFloatField;
  class QtVec3Field;
  class QtNamedIndexField;
  class QtIntegerField;
  class QtBooleanField;
 public:
  explicit QtPropertiesPanel(QtWindow* window_);
  void Update();
 private:
  QtWindow* window_;
  QFormLayout* layout_;
  QLabel* property_label_;

  void DisplayProperties(const catalyst::PropertyManager* property_manager);
};
class EditorWindow::QtWindow::QtPropertiesPanel::QtFloatField : public QWidget {
 public:
  QtFloatField(QtPropertiesPanel* property_panel,
               catalyst::FloatProperty* property);

 private:
  QtPropertiesPanel* property_panel_;
  catalyst::FloatProperty* property_;
  QDoubleSpinBox* spinbox_;

 private slots:
  void ValueChanged(double v);
};
class EditorWindow::QtWindow::QtPropertiesPanel::QtVec3Field : public QWidget {
 public:
  QtVec3Field(QtPropertiesPanel* property_panel,
              catalyst::Vec3Property* property);
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
class EditorWindow::QtWindow::QtPropertiesPanel::QtNamedIndexField : public QWidget {
 public:
  QtNamedIndexField(QtPropertiesPanel* property_panel,
               catalyst::NamedIndexProperty* property);

 private:
  QtPropertiesPanel* property_panel_;
  catalyst::NamedIndexProperty* property_;
  QComboBox* combobox_;
  int GetPropertyIndex(int combobox_index);
  void SetComboBoxIndex(int property_index);

 private slots:
  void ValueChanged(int v);
};
class EditorWindow::QtWindow::QtPropertiesPanel::QtIntegerField
    : public QWidget {
 public:
  QtIntegerField(QtPropertiesPanel* property_panel,
                 catalyst::IntegerProperty* property);

 private:
  QtPropertiesPanel* property_panel_;
  catalyst::IntegerProperty* property_;
  QSpinBox* spinbox_;

 private slots:
  void ValueChanged(int v);
};
class EditorWindow::QtWindow::QtPropertiesPanel::QtBooleanField
    : public QWidget {
 public:
  QtBooleanField(QtPropertiesPanel* property_panel,
                 catalyst::BooleanProperty* property);

 private:
  QtPropertiesPanel* property_panel_;
  catalyst::BooleanProperty* property_;
  QCheckBox* checkbox_;
  
 private slots:
  void ValueChanged(int state);
};
}