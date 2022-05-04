#include <editor/window/qtscenetree.h>

#include <iostream>

#include <QStandardItemModel>
#include <QDropEvent>

#include <editor/window/qtwindow.h>
#include <editor/window/qtpropertiespanel.h>
#include <editor/script/editorscript.h>
#include <catalyst/scene/scene.h>

namespace editor {
const std::string EditorWindow::QtWindow::QtSceneTree::QtSceneAddBox::kAddOptionNames[6] = {
    "Add...", "Empty", "Cube", "Teapot", "Bunny", "Directional Light"};
EditorWindow::QtWindow::QtSceneTree::QtSceneTree(QtWindow* window)
    : window_(window),
      layout_(new QGridLayout(this)),
      searchbox_(new QtSceneSearchBox(this)),
      treeview_(new QtSceneTreeView(this)),
      addbox_(new QtSceneAddBox(this)) {
  layout_->addWidget(searchbox_, 0, 0);
  layout_->addWidget(treeview_, 1, 0);
  layout_->addWidget(addbox_, 2, 0);
}
void EditorWindow::QtWindow::QtSceneTree::LoadScene() { treeview_->Populate(); }
void EditorWindow::QtWindow::QtSceneTree::Update() { treeview_->Populate(); }
EditorWindow::QtWindow::QtSceneTree::QtSceneSearchBox::QtSceneSearchBox(QtSceneTree* scene_tree)
    : scene_tree_(scene_tree) {}
void EditorWindow::QtWindow::QtSceneTree::QtSceneSearchBox::changeEvent(QEvent* ev) {
  scene_tree_->search_text_ = text().toStdString();
}
EditorWindow::QtWindow::QtSceneTree::QtSceneAddBox::QtSceneAddBox(QtSceneTree* scene_tree)
    : scene_tree_(scene_tree) {
  setEditable(false);
  for (uint32_t option_i = 0; option_i < static_cast<uint32_t>(AddOption::kCount); option_i++) {
    addItem(QString::fromStdString(kAddOptionNames[option_i]));
  }
  connect(this, &QtSceneTree::QtSceneAddBox::activated, this,
          &QtSceneTree::QtSceneAddBox::SelectCallback);
}
void EditorWindow::QtWindow::QtSceneTree::QtSceneAddBox::SelectCallback(int index) {
  ProcessAddCommand(static_cast<AddOption>(index));
  setCurrentIndex(static_cast<int>(AddOption::kNone));
}
void EditorWindow::QtWindow::QtSceneTree::QtSceneAddBox::ProcessAddCommand(AddOption a) {
  catalyst::Scene* scene = scene_tree_->window_->editor_->scene_;
  switch (a) {
    case AddOption::kNone:
      break;
    case AddOption::kEmpty: {
      scene->AddPrimitive(scene->root_, catalyst::PrimitiveType::kEmpty);
      scene_tree_->treeview_->Populate();
      break;
    }
    case AddOption::kCube: {
      scene->AddPrimitive(scene->root_, catalyst::PrimitiveType::kCube);
      scene_tree_->treeview_->Populate();
      break;
    }
    case AddOption::kTeapot: {
      scene->AddPrimitive(scene->root_, catalyst::PrimitiveType::kTeapot);
      scene_tree_->treeview_->Populate();
      break;
    }
    case AddOption::kBunny: {
      scene->AddPrimitive(scene->root_, catalyst::PrimitiveType::kBunny);
      scene_tree_->treeview_->Populate();
      break;
    }
    case AddOption::kDirectionalLight: {
      scene->AddDirectionalLight(scene->root_);
      scene_tree_->treeview_->Populate();
      break;
    }
  }
}
EditorWindow::QtWindow::QtSceneTree::QtSceneTreeView::QtSceneTreeView(QtSceneTree* scene_tree)
    : scene_tree_(scene_tree), tree_model_(nullptr) {
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  setDragDropMode(QAbstractItemView::DragDropMode::InternalMove);
}
void EditorWindow::QtWindow::QtSceneTree::QtSceneTreeView::Populate() {
  QItemSelectionModel* selection = selectionModel();
  setModel(nullptr);
  delete selection;
  delete tree_model_;
  tree_model_ = new QStandardItemModel();
  tree_model_->setHorizontalHeaderLabels({"Name"});
  QStandardItem* root_item = tree_model_->invisibleRootItem();
  for (catalyst::SceneObject* scene_child :
       scene_tree_->window_->editor_->scene_->root_->children_) 
    PopulateDfs(*(scene_child), root_item);
  setModel(tree_model_);
  setHeaderHidden(false);
}
void EditorWindow::QtWindow::QtSceneTree::QtSceneTreeView::selectionChanged(
  const QItemSelection& selected, const QItemSelection& deselected) {
  QtWindow* window = (scene_tree_->window_);
  std::vector<const catalyst::SceneObject*>& selection = window->object_selection_;
  selection.clear();
  for (const QModelIndex& s_index : selectedIndexes()) {
    catalyst::SceneObject* s_object = ModelIndexToSceneObject(s_index);
    if (s_object != nullptr) selection.push_back(s_object);
  }
  window->selection_updated_ = true;
  window->selection_type_ = SelectionType::kObject;
  scene_tree_->window_->properties_panel_->Update();
}
void EditorWindow::QtWindow::QtSceneTree::QtSceneTreeView::dropEvent(QDropEvent* drop_event) {
  DropIndicatorPosition drop_indicator = dropIndicatorPosition();
  QModelIndex drop_index = indexAt(drop_event->position().toPoint());
  catalyst::SceneObject* new_parent_object =
      ModelIndexToSceneObject(drop_index, scene_tree_->window_->editor_->scene_->root_);
  for (const QModelIndex& s_index : selectedIndexes()) {
    catalyst::SceneObject* selected_object = ModelIndexToSceneObject(s_index);
    if (selected_object!=nullptr) {
      if (selected_object == new_parent_object) continue;
      catalyst::SceneObject* selected_parent = selected_object->parent_;
      selected_parent->children_.erase(
          std::find(selected_parent->children_.begin(),
                    selected_parent->children_.end(), selected_object));
      selected_object->parent_ = new_parent_object;
      new_parent_object->children_.push_back(selected_object);
    }
  }
  QTreeView::dropEvent(drop_event);
}
void EditorWindow::QtWindow::QtSceneTree::QtSceneTreeView::keyReleaseEvent(
    QKeyEvent* key_event) {
  if (key_event->key() == Qt::Key::Key_Delete) {
    QModelIndex model_index = currentIndex();
    if (model_index.isValid()) {
      catalyst::SceneObject* scene_object =
          ModelIndexToSceneObject(model_index);
      if (scene_object != nullptr) {
        catalyst::SceneObject* parent = scene_object->parent_;
        parent->children_.erase(std::find(
            parent->children_.begin(), parent->children_.end(), scene_object));
        delete scene_object;
        key_event->accept();
        Populate();
      }
    }
  }
}
void EditorWindow::QtWindow::QtSceneTree::QtSceneTreeView::PopulateDfs(
    const catalyst::SceneObject& scene_object, QStandardItem* focus_item) {
  if (scene_object.external_) return;
  QStandardItem* focus_child =
      new QStandardItem(QString::fromStdString(scene_object.name_));
  focus_item->appendRow(focus_child);
  for (catalyst::SceneObject* scene_child:scene_object.children_) {
    PopulateDfs(*scene_child, focus_child);
  }
}
catalyst::SceneObject* EditorWindow::QtWindow::QtSceneTree::QtSceneTreeView::ModelIndexToSceneObject(
    const QModelIndex& model_index, catalyst::SceneObject* fallback) const {
  if (!model_index.isValid()) return fallback;
  QStandardItem* model_item = tree_model_->itemFromIndex(model_index);
  std::string model_text = model_item->text().toStdString();
  return scene_tree_->window_->editor_->scene_->GetObjectByName(model_text);
}
}  // namespace editor