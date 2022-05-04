#pragma once
#include <QWidget>
#include <QStandardItemModel>

#include <editor/window/qtwindow.h>

namespace editor {
class EditorWindow::QtWindow::QtResourcePanel : public QWidget {
 public:
  QtResourcePanel(QtWindow* window);
  void LoadScene();

 private:
  class QtResourceTreeView : public QTreeView{
   public:
    QtResourceTreeView(QtResourcePanel* resource_panel);

   protected:
    void selectionChanged(const QItemSelection& selected,
                          const QItemSelection& deselected) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

   private:
    QtResourcePanel* resource_panel_;
  };
  QtWindow* window_;
  QGridLayout* layout_;
  QtResourceTreeView* treeview_;
  QStandardItemModel* treemodel_;
  QAction* add_action_;
  QAction* duplicate_action_;
  catalyst::Scene* scene_;

  void Populate();
  catalyst::Resource* ModelIndexToResource(
      const QModelIndex& model_index, catalyst::Resource* fallback = nullptr) const;

 private slots:
  void DuplicateResource();
  void AddResource();
};
}