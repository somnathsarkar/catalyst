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
  std::vector<const catalyst::SceneObject*>& selection = window_->selection_;
  uint32_t selection_len = static_cast<uint32_t>(selection.size());
  std::ostringstream label_stream;
  if (selection_len != 1) {
    label_stream << selection_len << " object(s) selected.";
  } else {
    const catalyst::SceneObject* focus = selection[0];
    label_stream << focus->name_;
    for (uint32_t prop_i = 0; prop_i < focus->property_manager_.PropertyCount();
         prop_i++) {
      catalyst::Property* prop = focus->property_manager_.GetProperty(prop_i);
      switch (prop->type_) {
        case catalyst::PropertyType::kVec3: {
          QtVec3Field* field =
              new QtVec3Field(this, static_cast<catalyst::Vec3Property*>(prop));
          layout_->addRow(QString::fromStdString(prop->name_), field);
          break;
        }
        default:
          ASSERT(false, "Unhandled property type!");
          break;
      }
    }
  }
  property_label_->setText(QString::fromStdString(label_stream.str()));
}
EditorWindow::QtWindow::QtPropertiesPanel::QtVec3Field::QtVec3Field(
    QtPropertiesPanel* property_panel,
    catalyst::Vec3Property* property)
    : property_panel_(property_panel),property_(property) {
  layout_ = new QHBoxLayout(this);
  spinboxes_ = new QDoubleSpinBox[3];
  glm::vec3 initial_vec = property_->getter_();
  float vec_array[3] = {initial_vec.x, initial_vec.y, initial_vec.z};
  for (uint32_t i = 0; i < 3; i++) {
    spinboxes_[i].setMinimum(property_->min_value_);
    spinboxes_[i].setMaximum(property_->max_value_);
    spinboxes_[i].setSingleStep(0.1);
    spinboxes_[i].setValue(vec_array[i]);
    layout_->addWidget(&spinboxes_[i]);
    QObject::connect(&spinboxes_[i], &QDoubleSpinBox::valueChanged, this,
                     &QtVec3Field::ValueChanged);
  }
}
EditorWindow::QtWindow::QtPropertiesPanel::QtVec3Field::~QtVec3Field() {
  delete[] spinboxes_;
}
glm::vec3 EditorWindow::QtWindow::QtPropertiesPanel::QtVec3Field::GetVec3()
    const {
  return glm::vec3(static_cast<float>(spinboxes_[0].value()),
                   static_cast<float>(spinboxes_[1].value()),
                   static_cast<float>(spinboxes_[2].value()));
}
void EditorWindow::QtWindow::QtPropertiesPanel::QtVec3Field::ValueChanged(
  double v) {
  glm::vec3 current_value = GetVec3();
  property_->setter_(current_value);
  property_panel_->window_->selection_updated_ = true;
}
}  // namespace editor