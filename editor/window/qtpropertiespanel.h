#pragma once
#include <vector>

#include <QWidget>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QColorDialog>
#include <QPushButton>

#include <catalyst/scene/propertymanager.h>
#include <editor/window/qtwindow.h>

namespace editor {
class EditorWindow::QtWindow::QtPropertiesPanel final : public QWidget{
  class QtFloatField;
  class QtVec3SpinboxField;
  class QtVec3ColorField;
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
  QHBoxLayout* layout_;
  QDoubleSpinBox* spinbox_;

 private slots:
  void ValueChanged(double v);
};
class EditorWindow::QtWindow::QtPropertiesPanel::QtVec3SpinboxField : public QWidget {
 public:
  QtVec3SpinboxField(QtPropertiesPanel* property_panel,
              catalyst::Vec3Property* property);
  ~QtVec3SpinboxField();

 private:
  QtPropertiesPanel* property_panel_;
  catalyst::Vec3Property* property_;
  QHBoxLayout* layout_;
  QDoubleSpinBox* spinboxes_;
  glm::vec3 GetVec3() const;

 private slots:
  void ValueChanged(double v);
};
class EditorWindow::QtWindow::QtPropertiesPanel::QtVec3ColorField
    : public QWidget {
 public:
  QtVec3ColorField(QtPropertiesPanel* property_panel,
                     catalyst::Vec3Property* property);

 private:
  QtPropertiesPanel* property_panel_;
  catalyst::Vec3Property* property_;
  QHBoxLayout* layout_;
  QPushButton* color_button;
  QColorDialog* color_dialog;
  glm::vec3 GetVec3(const QColor& color) const;
  void UpdateButton(const QColor& color);

 private slots:
  void ValueChanged(QColor v);
  void ButtonPressed();
};
class EditorWindow::QtWindow::QtPropertiesPanel::QtNamedIndexField : public QWidget {
 public:
  QtNamedIndexField(QtPropertiesPanel* property_panel,
               catalyst::NamedIndexProperty* property);

 private:
  QtPropertiesPanel* property_panel_;
  catalyst::NamedIndexProperty* property_;
  QHBoxLayout* layout_;
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
  QHBoxLayout* layout_;
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
  QHBoxLayout* layout_;
  QCheckBox* checkbox_;
  
 private slots:
  void ValueChanged(int state);
};
}