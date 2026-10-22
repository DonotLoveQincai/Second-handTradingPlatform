#include "ui/mainwindow.h"
#include "ui_mainwindow.h"

#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

#include "data/db_manager.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(QStringLiteral("二手交易平台"));
    resize(1100, 700);

    buildUi();
    connectBus();

    statusBar()->showMessage(QStringLiteral("数据库：") + DbManager::dbPath());
}

MainWindow::~MainWindow()
{
    delete ui;
}

QWidget* MainWindow::makePlaceholder(const QString& text)
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    auto* label = new QLabel(QStringLiteral("【%1】\n（待实现）").arg(text), page);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    return page;
}

void MainWindow::buildUi()
{
    // 五个页面，顺序就是导航顺序。第 2 周把 makePlaceholder 换成真页面。
    ui->stackedWidget->addWidget(makePlaceholder(QStringLiteral("商品检索")));
    ui->stackedWidget->addWidget(makePlaceholder(QStringLiteral("我的商品")));
    ui->stackedWidget->addWidget(makePlaceholder(QStringLiteral("我的订单")));
    ui->stackedWidget->addWidget(makePlaceholder(QStringLiteral("交易记录")));
    ui->stackedWidget->addWidget(makePlaceholder(QStringLiteral("批量导入")));

    ui->navList->addItem(QStringLiteral("商品检索"));
    ui->navList->addItem(QStringLiteral("我的商品"));
    ui->navList->addItem(QStringLiteral("我的订单"));
    ui->navList->addItem(QStringLiteral("交易记录"));
    ui->navList->addItem(QStringLiteral("批量导入"));

    ui->navList->setCurrentRow(0);
    ui->stackedWidget->setCurrentIndex(0);

    connect(ui->navList, &QListWidget::currentRowChanged,
        ui->stackedWidget, &QStackedWidget::setCurrentIndex);
}

void MainWindow::connectBus()
{
    // 第 0 周还没有别的对象，留空。第 2 周在这里接服务与页面。
}
