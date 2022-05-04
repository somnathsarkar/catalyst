#pragma once
#include <string>
#include <map>

#include <QWidget>
#include <QGridLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QTreeView>
#include <QStandardItemModel>

#include <catalyst/scene/sceneobject.h>
#include <editor/window/editorwindow.h>
#include <editor/window/qtwindow.h>

namespace editor {
class EditorWindow::QtWindow::QtSceneTree final : public QWidget {
 public:
  explicit QtSceneTree(QtWindow* window);
  void LoadScene();
  void Update();
 private:
  class QtSceneSearchBox : public QLineEdit {
   public:
    explicit QtSceneSearchBox(EditorWindow::QtWindow::QtSceneTree* scene_tree);
   protected:
    void changeEvent(QEvent* ev) override;
   private:
    QtSceneTree* scene_tree_;
  };
  class QtSceneAddBox : public QComboBox {
   public:
    explicit QtSceneAddBox(QtSceneTree* scene_tree);
   private:
     enum class AddOption : uint32_t{
       kNone = 0,
       kEmpty = 1,
       kCube = 2,
       kTeapot = 3,
       kBunny = 4,
       kDirectionalLight = 5,
       kCount = 6,
    };
    static const std::string kAddOptionNames[6];
     QtSceneTree* scene_tree_;
     void ProcessAddCommand(AddOption a);
   private slots:
    void SelectCallback(int index);
  };
  class QtSceneTreeView final: public QTreeView {
   public:
    explicit QtSceneTreeView(QtSceneTree* scene_tree);
    void Populate();
   protected:
    void selectionChanged(const QItemSelection& selected,
                          const QItemSelection& deselected) override;
    void dropEvent(QDropEvent* drop_event) override;
    void keyReleaseEvent(QKeyEvent* key_event) override;
   private:
    QtSceneTree* scene_tree_;
    QStandardItemModel* tree_model_;
    void PopulateDfs(const catalyst::SceneObject& scene_object, QStandardItem* focus_item);
    catalyst::SceneObject* ModelIndexToSceneObject(const QModelIndex& model_index, catalyst::SceneObject* fallback = nullptr) const;
  };
  QtWindow* window_;
  QGridLayout* layout_;
  QtSceneSearchBox* searchbox_;
  QtSceneTreeView* treeview_;
  QtSceneAddBox* addbox_;

  std::string search_text_;
};
}