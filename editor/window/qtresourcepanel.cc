#include <editor/window/qtresourcepanel.h>

#include <iostream>
#include <QMenu>

#include <editor/window/qtpropertiespanel.h>
#include <editor/window/qtwindow.h>
#include <editor/window/qtscenetree.h>
#include <catalyst/filesystem/importer.h>

namespace editor {
EditorWindow::QtWindow::QtResourcePanel::QtResourcePanel(QtWindow* window) : QWidget(window), window_(window), treemodel_(nullptr), scene_(nullptr) {
  layout_ = new QGridLayout(this);
  treeview_ = new QtResourceTreeView(this);
  layout_->addWidget(treeview_, 0, 0);

  duplicate_action_ = new QAction("Duplicate Resource", this);
  duplicate_action_->setShortcut(Qt::CTRL + Qt::Key_D);
  QObject::connect(duplicate_action_, &QAction::triggered, this,
                   &QtResourcePanel::DuplicateResource);
  addAction(duplicate_action_);

  add_action_ = new QAction("Add to Scene", this);
  add_action_->setShortcut(Qt::CTRL + Qt::Key_A);
  QObject::connect(add_action_, &QAction::triggered, this,
                   &QtResourcePanel::AddResource);

  open_action_ = new QAction("Import Resource", this);
  open_action_->setShortcut(Qt::CTRL + Qt::Key_O);
  QObject::connect(open_action_, &QAction::triggered, this,
                   &QtResourcePanel::ImportResource);
  addAction(open_action_);

  open_dialog_ = new QFileDialog(this);
  open_dialog_->setFileMode(QFileDialog::FileMode::ExistingFile);
  open_dialog_->setViewMode(QFileDialog::ViewMode::Detail);
}
void EditorWindow::QtWindow::QtResourcePanel::LoadScene() {
  catalyst::Scene* scene = window_->editor_->scene_;
  scene_ = scene;
  Populate();
}
void EditorWindow::QtWindow::QtResourcePanel::Populate() {
  QItemSelectionModel* selection_model = treeview_->selectionModel();
  treeview_->setModel(nullptr);
  delete selection_model;
  delete treemodel_;
  treemodel_ = new QStandardItemModel();
  treemodel_->setHorizontalHeaderLabels({"Name", "Type"});
  uint32_t row_count = 0;
  for (uint32_t mesh_i = 0;
       mesh_i < static_cast<uint32_t>(scene_->meshes_.size()); mesh_i++) {
    QStandardItem* item_name = new QStandardItem(
        QString::fromStdString(scene_->meshes_[mesh_i]->name_));
    QStandardItem* item_type = new QStandardItem(QString::fromStdString(
        catalyst::kResourceTypeNames[static_cast<uint32_t>(
            catalyst::ResourceType::kMesh)]));
    item_type->setEditable(false);
    treemodel_->setItem(row_count, 0, item_name);
    treemodel_->setItem(row_count, 1, item_type);
    row_count++;
  }
  for (uint32_t mat_i = 0;
       mat_i < static_cast<uint32_t>(scene_->materials_.size()); mat_i++) {
    QStandardItem* item_name = new QStandardItem(QString::fromStdString(scene_->materials_[mat_i]->name_));
    QStandardItem* item_type = new QStandardItem(QString::fromStdString(
        catalyst::kResourceTypeNames[static_cast<uint32_t>(
            catalyst::ResourceType::kMaterial)]));
    item_type->setEditable(false);
    treemodel_->setItem(row_count, 0, item_name);
    treemodel_->setItem(row_count, 1, item_type);
    row_count++;
  }
  for (uint32_t tex_i = 0;
       tex_i < static_cast<uint32_t>(scene_->textures_.size()); tex_i++) {
    QStandardItem* item_name = new QStandardItem(
        QString::fromStdString(scene_->textures_[tex_i]->name_));
    QStandardItem* item_type = new QStandardItem(QString::fromStdString(
        catalyst::kResourceTypeNames[static_cast<uint32_t>(
            catalyst::ResourceType::kTexture)]));
    item_type->setEditable(false);
    treemodel_->setItem(row_count, 0, item_name);
    treemodel_->setItem(row_count, 1, item_type);
    row_count++;
  }
  treeview_->setModel(treemodel_);
}
catalyst::Resource*
EditorWindow::QtWindow::QtResourcePanel::ModelIndexToResource(
    const QModelIndex& model_index, catalyst::Resource* fallback) const {
  if (!model_index.isValid()) return fallback;
  if (model_index.column() != 0) return fallback;
  QStandardItem* model_item = treemodel_->itemFromIndex(model_index);
  std::string model_text = model_item->text().toStdString();
  return scene_->GetResourceByName(model_text);
}
EditorWindow::QtWindow::QtResourcePanel::QtResourceTreeView::QtResourceTreeView(
    QtResourcePanel* resource_panel)
    : resource_panel_(resource_panel) {
  setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
}
void EditorWindow::QtWindow::QtResourcePanel::QtResourceTreeView::
    selectionChanged(const QItemSelection& selected,
                     const QItemSelection& deselected) {
  resource_panel_->window_->selection_updated_ = true;
  resource_panel_->window_->selection_type_ = SelectionType::kResource;
  resource_panel_->window_->resource_selection_.clear();
  for (const QModelIndex& model_index : selected.indexes()) {
    catalyst::Resource* resource =
        resource_panel_->ModelIndexToResource(model_index);
    if(resource!=nullptr)
      resource_panel_->window_->resource_selection_.push_back(resource);
  }
  resource_panel_->window_->properties_panel_->Update();
  QTreeView::selectionChanged(selected, deselected);
}
void EditorWindow::QtWindow::QtResourcePanel::QtResourceTreeView::
    contextMenuEvent(QContextMenuEvent* event) {
  QMenu context_menu("Resource Menu", this);
  context_menu.addAction(resource_panel_->add_action_);
  context_menu.addAction(resource_panel_->duplicate_action_);
  context_menu.exec(viewport()->mapToGlobal(event->pos()));
}
void EditorWindow::QtWindow::QtResourcePanel::DuplicateResource(){
  QModelIndex model_index = treeview_->currentIndex();
  if (model_index.isValid()) model_index = model_index.siblingAtColumn(0);
  catalyst::Resource* resource = ModelIndexToResource(model_index);
  if (resource != nullptr) {
    scene_->DuplicateResource(resource);
    Populate();
  }
}
void EditorWindow::QtWindow::QtResourcePanel::AddResource() {
  QModelIndex model_index = treeview_->currentIndex();
  if (model_index.isValid()) model_index = model_index.siblingAtColumn(0);
  catalyst::Resource* resource = ModelIndexToResource(model_index);
  if (resource != nullptr) {
    scene_->AddResourceToScene(resource);
    window_->scene_tree_->Update();
  }
}
void EditorWindow::QtWindow::QtResourcePanel::ImportResource() {
  QStringList filenames;
  if (open_dialog_->exec()) filenames = open_dialog_->selectedFiles();
  for (const QString& file : filenames) {
    catalyst::Importer::AddResources(*scene_,file.toStdString());
  }
  Populate();
}
}  // namespace editor