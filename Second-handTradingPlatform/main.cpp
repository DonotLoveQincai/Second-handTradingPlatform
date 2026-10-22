#include <QApplication>
#include <QMessageBox>

#include "data/db_manager.h"
#include "ui/mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("二手交易平台"));

    // 1) 先把数据库准备好，失败就直接退出，不让用户看到半个窗口
    QString err;
    if (!DbManager::init(&err)) {
        QMessageBox::critical(nullptr, QStringLiteral("初始化失败"), err);
        return 1;
    }
    DbManager::dumpSchema();   // 调试用，正式打包前可注释掉

    // 2) 主窗口（第 0 周只有占位页，第 2 周才会接上真正的页面）
    MainWindow w;
    w.show();

    return app.exec();
}
