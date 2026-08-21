#pragma once

#include <QMainWindow>

namespace renderlab {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void createMenus();
    void createDockPanels();
};

} // namespace renderlab

