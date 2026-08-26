#include "app/MainWindow.h"

#include "core/AppInfo.h"
#include "renderer/RenderViewport.h"

#include <QApplication>
#include <QDockWidget>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QTreeWidget>
#include <QVariant>

namespace renderlab {
namespace {

QDockWidget* makeDock(const QString& title, QWidget* content, QWidget* parent) {
    auto* dock = new QDockWidget(title, parent);
    dock->setObjectName(title);
    dock->setWidget(content);
    return dock;
}

QString componentSummary(const SceneDocument& scene, const EntityId entity) {
    QStringList components{QStringLiteral("Transform")};

    if (scene.tryGetMeshRenderer(entity) != nullptr) {
        components.push_back(QStringLiteral("Mesh Renderer"));
    }
    if (scene.tryGetCamera(entity) != nullptr) {
        components.push_back(QStringLiteral("Camera"));
    }
    if (scene.tryGetLight(entity) != nullptr) {
        components.push_back(QStringLiteral("Light"));
    }

    return components.join(QStringLiteral(", "));
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setObjectName("RenderLabMainWindow");
    setWindowTitle(QStringLiteral("%1 %2")
                       .arg(QString::fromUtf8(applicationName()))
                       .arg(QString::fromUtf8(applicationVersion())));
    resize(1440, 900);

    createDefaultScene();

    viewport_ = new RenderViewport(this);
    viewport_->setScene(&scene_);
    setCentralWidget(viewport_);

    createMenus();
    createDockPanels();
    statusBar()->showMessage(tr("Ready — SceneDocument initialized"));
}

void MainWindow::createDefaultScene() {
    const EntityId camera = scene_.createEntity("Camera");
    scene_.addCamera(camera).primary = true;
    scene_.tryGetTransform(camera)->position = {0.0F, 0.0F, 5.0F};

    const EntityId light = scene_.createEntity("Directional Light");
    scene_.addLight(light);
    scene_.tryGetTransform(light)->rotationDegrees = {-45.0F, 45.0F, 0.0F};

    const EntityId cube = scene_.createEntity("Cube");
    scene_.addMeshRenderer(cube).meshAsset = BuiltinCubeMeshAsset;
    scene_.tryGetTransform(cube)->rotationDegrees = {-20.0F, 30.0F, 0.0F};
}

void MainWindow::createMenus() {
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("New Scene"));
    fileMenu->addAction(tr("Import Model..."));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Exit"), qApp, &QApplication::quit);

    menuBar()->addMenu(tr("&Edit"));

    auto* createMenu = menuBar()->addMenu(tr("&Create"));
    auto* emptyAction = createMenu->addAction(tr("Empty"));
    connect(emptyAction, &QAction::triggered, this, [this] {
        scene_.createEntity("Empty");
        refreshSceneTree();
    });

    auto* cubeAction = createMenu->addAction(tr("Cube"));
    connect(cubeAction, &QAction::triggered, this, [this] {
        const EntityId cube = scene_.createEntity("Cube");
        scene_.addMeshRenderer(cube).meshAsset = BuiltinCubeMeshAsset;
        refreshSceneTree();
        viewport_->requestRender();
    });

    menuBar()->addMenu(tr("&Render"));
    menuBar()->addMenu(tr("&Learn"));
}

void MainWindow::createDockPanels() {
    sceneTree_ = new QTreeWidget(this);
    sceneTree_->setHeaderLabel(tr("Scene"));
    connect(sceneTree_, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem* current) {
                const EntityId entity = current == nullptr
                                            ? NullEntity
                                            : current->data(0, Qt::UserRole).toULongLong();
                updateInspector(entity);
            });
    addDockWidget(Qt::LeftDockWidgetArea, makeDock(tr("Scene"), sceneTree_, this));

    inspectorLabel_ = new QLabel(tr("No object selected"), this);
    inspectorLabel_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    inspectorLabel_->setMargin(8);
    addDockWidget(Qt::RightDockWidgetArea,
                  makeDock(tr("Inspector"), inspectorLabel_, this));

    auto* output = new QListWidget(this);
    output->addItem(tr("RenderLab initialized"));
    output->addItem(tr("Default SceneDocument created"));
    addDockWidget(Qt::BottomDockWidgetArea, makeDock(tr("Console"), output, this));

    refreshSceneTree();
}

void MainWindow::refreshSceneTree() {
    if (sceneTree_ == nullptr) {
        return;
    }

    sceneTree_->clear();
    for (const EntityId root : scene_.rootEntities()) {
        addEntityToTree(root, nullptr);
    }
    sceneTree_->expandAll();
    updateInspector(NullEntity);
}

void MainWindow::addEntityToTree(const EntityId entity, QTreeWidgetItem* parentItem) {
    const EntityMetadata* metadata = scene_.tryGetEntity(entity);
    if (metadata == nullptr) {
        return;
    }

    auto* item = new QTreeWidgetItem({QString::fromStdString(metadata->name)});
    item->setData(0, Qt::UserRole, QVariant::fromValue<qulonglong>(entity));

    if (parentItem == nullptr) {
        sceneTree_->addTopLevelItem(item);
    } else {
        parentItem->addChild(item);
    }

    for (const EntityId child : metadata->children) {
        addEntityToTree(child, item);
    }
}

void MainWindow::updateInspector(const EntityId entity) {
    if (inspectorLabel_ == nullptr) {
        return;
    }

    const EntityMetadata* metadata = scene_.tryGetEntity(entity);
    if (metadata == nullptr) {
        inspectorLabel_->setText(tr("No object selected"));
        return;
    }

    inspectorLabel_->setText(
        tr("Name: %1\nEntity ID: %2\nComponents: %3")
            .arg(QString::fromStdString(metadata->name))
            .arg(entity)
            .arg(componentSummary(scene_, entity)));
}

} // namespace renderlab

