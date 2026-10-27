#include <QApplication>
#include <QMessageBox>

#include "core/session.h"
#include "data/db_manager.h"
#include "data/user_dao.h"
#include "services/auth_service.h"
#include "services/trade_state_machine.h"
#include "ui/mainwindow.h"
#include "ui/pages/auth/login_dialog.h"

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

    // 2) 状态机自检：穷举 5 状态 × 4 动作，合法边必须恰好 5 条。
    //    写错状态表时一启动就会在调试器里断言，早失败好过演示时出错。
    TradeStateMachine::selfCheck();

    // 3) 先弹登录框；用户点 X 直接退出，不要进到一个没登录的主界面
    UserDao userDao;
    AuthService authService(&userDao);
    LoginDialog loginDlg(&authService);
    if (loginDlg.exec() != QDialog::Accepted)
        return 0;

    // 4) 主窗口：登录成功后把用户交给它，标题栏显示昵称
    MainWindow w;
    w.setCurrentUser(Session::instance().currentUser());
    w.show();

    return app.exec();
}
