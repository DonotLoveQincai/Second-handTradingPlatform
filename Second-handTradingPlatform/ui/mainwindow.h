#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void buildUi();        // 建导航项、放页面
    void connectBus();     // 全项目唯一 connect 的地方（第 2 周填满）

    QWidget* makePlaceholder(const QString& text);   // 占位页，第 2 周会被真页面替换

    Ui::MainWindow* ui;
};

