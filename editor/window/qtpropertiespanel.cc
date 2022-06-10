#include <editor/window/qtpropertiespanel.h>

#include <vector>
#include <sstream>

#include <editor/script/editorscript.h>
#include <catalyst/dev/dev.h>

namespace editor {
EditorWindow::QtWindow::QtPropertiesPanel::QtPropertiesPanel(
    QtWindow* window) : window_(window) {
  layout_ = new QFormLayout(this);
  layout_->setLabelAlignment(Qt::AlignVCenter);
  property_label_ = new QLabel();
  property_label_->setAlignment(Qt::AlignCenter);
  layout_->addWidget(property_label_);
}

void EditorWindow::QtWindow::QtPropertiesPanel::Update() {
  // Destroy
  while (layout_->rowCount() > 1) layout_->removeRow(1);
  // Rebuild
  uint32_t selection_len = 0;
  std::ostringstream label_stream;
  const catalyst::PropertyManager* property_manager = nullptr;
  switch (window_->selection_type_) {
    case SelectionType::kObject: {
      std::vector<const catalyst::SceneObject*>& selection =
          window_->object_selection_;
      selection_len = static_cast<uint32_t>(selection.size());
      if (selection_len != 1) {
        label_stream << selection_len << " object(s) selected.";
      } else {
        const catalyst::SceneObject* focus = selection[0];
        label_stream << focus->name_;
        property_manager = &(focus->property_manager_);
      }
      break;
    }
    case SelectionType::kResource: {
      std::vector<const catalyst::Resource*>& selection =
          window_->resource_selection_;
      selection_len = static_cast<uint32_t>(selection.size());
      if (selection_len != 1) {
        label_stream << selection_len << " resource(s) selected.";
      } else {
        const catalyst::Resource* focus = selection[0];
        label_stream << focus->name_;
        property_manager = &(focus->property_manager_);
      }
      break;
    }
    default: {
      ASSERT(false, "Unhandled Selection Type!");
      break;
    }
  }
  property_label_->setText(QString::fromStdString(label_stream.str()));
  if (selection_len == 1) {
    DisplayProperties(property_manager);
  }
}
void EditorWindow::QtWindow::QtPropertiesPanel::DisplayProperties(
    const catalyst::PropertyManager* property_manager) {
  for (uint32_t prop_i = 0; prop_i < property_manager->PropertyCount();
       prop_i++) {
    catalyst::Property* prop = property_manager->GetProperty(prop_i);
    switch (prop->type_) {
      case catalyst::PropertyType::kFloat: {
        QtFloatField* field = new QtFloatField(this, static_cast<catalyst::FloatProperty*>(prop));
        layout_->addRow(QString::fromStdString(prop->name_), field);
        break;
      }
      case catalyst::PropertyType::kVec3: {
        catalyst::Vec3Property* vec3_prop =
            static_cast<catalyst::Vec3Property*>(prop);
        switch (vec3_prop->style_) {
          case catalyst::Vec3PropertyStyle::kSpinbox: {
            QtVec3SpinboxField* field = new QtVec3SpinboxField(this, vec3_prop);
            layout_->addRow(QString::fromStdString(prop->name_), field);
            break;
          }
          case catalyst::Vec3PropertyStyle::kColor: {
            QtVec3ColorField* field = new QtVec3ColorField(this, vec3_prop);
            layout_->addRow(QString::fromStdString(prop->name_), field);
            break;
          }
          default:
            ASSERT(false, "Unhandled Vec3 Property type!");
            break;
        }
        break;
      }
      case catalyst::PropertyType::kNamedIndex: {
        QtNamedIndexField* field = new QtNamedIndexField(
            this, static_cast<catalyst::NamedIndexProperty*>(prop));
        layout_->addRow(QString::fromStdString(prop->name_), field);
        break;
      }
      case catalyst::PropertyType::kInteger: {
        QtIntegerField* field = new QtIntegerField(
            this, static_cast<catalyst::IntegerProperty*>(prop));
        layout_->addRow(QString::fromStdString(prop->name_), field);
        break;
      }
      case catalyst::PropertyType::kBoolean: {
        QtBooleanField* field = new QtBooleanField(
            this, static_cast<catalyst::BooleanProperty*>(prop));
        layout_->addRow(QString::fromStdString(prop->name_), field);
        break;
      }
      default:
        ASSERT(false, "Unhandled property type!");
        break;
    }
  }
}
EditorWindow::QtWindow::QtPropertiesPanel::QtVec3SpinboxField::QtVec3SpinboxField(
    QtPropertiesPanel* property_panel,
    catalyst::Vec3Property* property)
    : property_panel_(property_panel),property_(property) {
  layout_ = new QHBoxLayout(this);
  layout_->setContentsMargins(0, 0, 0, 0);
  spinboxes_ = new QDoubleSpinBox[3];
  glm::vec3 initial_vec = property_->getter_();
  float vec_array[3] = {initial_vec.x, initial_vec.y, initial_vec.z};
  for (uint32_t i = 0; i < 3; i++) {
    spinboxes_[i].setMinimum(property_->min_value_);
    spinboxes_[i].setMaximum(property_->max_value_);
    spinboxes_[i].setSingleStep(0.1);
    spinboxes_[i].setValue(vec_array[i]);
    QObject::connect(&spinboxes_[i], &QDoubleSpinBox::valueChanged, this,
                     &QtVec3SpinboxField::ValueChanged);
    layout_->addWidget(&spinboxes_[i]);
  }
}
EditorWindow::QtWindow::QtPropertiesPanel::QtVec3SpinboxField::~QtVec3SpinboxField() {
  delete[] spinboxes_;
}
glm::vec3 EditorWindow::QtWindow::QtPropertiesPanel::QtVec3SpinboxField::GetVec3()
    const {
  return glm::vec3(static_cast<float>(spinboxes_[0].value()),
                   static_cast<float>(spinboxes_[1].value()),
                   static_cast<float>(spinboxes_[2].value()));
}
void EditorWindow::QtWindow::QtPropertiesPanel::QtVec3SpinboxField::ValueChanged(
  double v) {
  glm::vec3 current_value = GetVec3();
  property_->setter_(current_value);
  property_panel_->window_->selection_updated_ = true;
}
EditorWindow::QtWindow::QtPropertiesPanel::QtFloatField::QtFloatField(
    QtPropertiesPanel* property_panel, catalyst::FloatProperty * property)
    : property_panel_(property_panel), property_(property) {
  layout_ = new QHBoxLayout(this);
  layout_->setContentsMargins(0, 0, 0, 0);
  spinbox_ = new QDoubleSpinBox(this);
  float initial_value = property_->getter_();
  spinbox_->setMinimum(property_->min_value_);
  spinbox_->setMaximum(property_->max_value_);
  spinbox_->setSingleStep(0.01);
  spinbox_->setDecimals(6);
  spinbox_->setValue(initial_value);
  QObject::connect(spinbox_, &QDoubleSpinBox::valueChanged, this,
                    &QtFloatField::ValueChanged);
  layout_->addWidget(spinbox_);
}
void EditorWindow::QtWindow::QtPropertiesPanel::QtFloatField::ValueChanged(
    double v) {
  float new_value = static_cast<float>(v);
  property_->setter_(new_value);
}
EditorWindow::QtWindow::QtPropertiesPanel::QtNamedIndexField::QtNamedIndexField(
    QtPropertiesPanel* property_panel, catalyst::NamedIndexProperty* property)
    : property_panel_(property_panel), property_(property) {
  layout_ = new QHBoxLayout(this);
  layout_->setContentsMargins(0, 0, 0, 0);
  combobox_ = new QComboBox(this);
  std::vector<std::string> name_list = property_->name_getter_();
  if (property_->style_ == catalyst::NamedIndexPropertyStyle::kAllowNone)
    combobox_->addItem("None");
  for (const std::string& name : name_list) {
    combobox_->addItem(QString::fromStdString(name));
  }
  SetComboBoxIndex(property_->getter_());
  QObject::connect(combobox_, &QComboBox::currentIndexChanged, this,
          &QtNamedIndexField::ValueChanged);
  layout_->addWidget(combobox_);
}
int EditorWindow::QtWindow::QtPropertiesPanel::QtNamedIndexField::
    GetPropertyIndex(int combobox_index) {
  int property_index = combobox_index;
  if (property_->style_ == catalyst::NamedIndexPropertyStyle::kAllowNone)
    property_index--;
  return property_index;
}
void EditorWindow::QtWindow::QtPropertiesPanel::QtNamedIndexField::
    SetComboBoxIndex(int property_index) {
  int combobox_index = property_index;
  if (property_->style_ == catalyst::NamedIndexPropertyStyle::kAllowNone)
    combobox_index++;
  combobox_->setCurrentIndex(combobox_index);
}
void EditorWindow::QtWindow::QtPropertiesPanel::QtNamedIndexField::ValueChanged(
  int v) {
  int new_value = GetPropertyIndex(v);
  property_->setter_(new_value);
}
EditorWindow::QtWindow::QtPropertiesPanel::QtIntegerField::QtIntegerField(
    QtPropertiesPanel* property_panel, catalyst::IntegerProperty* property)
    : property_panel_(property_panel), property_(property) {
  layout_ = new QHBoxLayout(this);
  layout_->setContentsMargins(0, 0, 0, 0);
  spinbox_ = new QSpinBox(this);
  float initial_value = property_->getter_();
  spinbox_->setMinimum(property_->min_value_);
  spinbox_->setMaximum(property_->max_value_);
  spinbox_->setValue(initial_value);
  QObject::connect(spinbox_, &QSpinBox::valueChanged, this,
                   &QtIntegerField::ValueChanged);
  layout_->addWidget(spinbox_);
}
void EditorWindow::QtWindow::QtPropertiesPanel::QtIntegerField::ValueChanged(
    int new_value) {
  property_->setter_(new_value);
}
EditorWindow::QtWindow::QtPropertiesPanel::QtBooleanField::QtBooleanField(
    QtPropertiesPanel* property_panel, catalyst::BooleanProperty* property)
    : property_panel_(property_panel), property_(property) {
  layout_ = new QHBoxLayout(this);
  layout_->setContentsMargins(0, 0, 0, 0);
  checkbox_ = new QCheckBox("", this);
  bool initial_value = property_->getter_();
  if (initial_value)
    checkbox_->setCheckState(Qt::CheckState::Checked);
  else
    checkbox_->setCheckState(Qt::CheckState::Unchecked);
  QObject::connect(checkbox_, &QCheckBox::stateChanged, this,
                   &QtBooleanField::ValueChanged);
  layout_->addWidget(checkbox_);
}
void EditorWindow::QtWindow::QtPropertiesPanel::QtBooleanField::ValueChanged(
  int state) {
  bool new_value = (state == Qt::CheckState::Checked) ? true : false;
  property_->setter_(new_value);
}
EditorWindow::QtWindow::QtPropertiesPanel::QtVec3ColorField::QtVec3ColorField(
    QtPropertiesPanel* property_panel, catalyst::Vec3Property* property)
    : property_panel_(property_panel), property_(property) {
  layout_ = new QHBoxLayout(this);
  layout_->setContentsMargins(0, 0, 0, 0);
  color_button = new QPushButton("Change color...", this);
  color_dialog = new QColorDialog(this);
  glm::vec3 value = property_->getter_();
  QColor initial_color = QColor::fromRgbF(value.r, value.g, value.b);
  UpdateButton(initial_color);
  color_button->setText(
      color_dialog->currentColor().name(QColor::NameFormat::HexRgb).toUpper());
  QObject::connect(color_dialog, &QColorDialog::currentColorChanged, this,
                   &QtVec3ColorField::ValueChanged);
  QObject::connect(color_button, &QAbstractButton::released, this,
                   &QtVec3ColorField::ButtonPressed);
  layout_->addWidget(color_button);
}
glm::vec3 EditorWindow::QtWindow::QtPropertiesPanel::QtVec3ColorField::GetVec3(const QColor& color)
    const {
  glm::vec3 ret = {color.redF(), color.greenF(), color.blueF()};
  return ret;
}
void EditorWindow::QtWindow::QtPropertiesPanel::QtVec3ColorField::UpdateButton(
    const QColor& color){
  QPixmap pixmap(100, 100);
  pixmap.fill(color);
  QIcon icon(pixmap);
  color_button->setIcon(icon);
  color_button->setText(color.name(QColor::NameFormat::HexRgb).toUpper());
}
void EditorWindow::QtWindow::QtPropertiesPanel::QtVec3ColorField::ValueChanged(
  QColor v) {
  glm::vec3 new_color = GetVec3(v);
  UpdateButton(v);
  property_->setter_(new_color);
}
void EditorWindow::QtWindow::QtPropertiesPanel::QtVec3ColorField::
ButtonPressed() {
  color_dialog->open();
}
}  // namespace editor