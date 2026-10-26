#pragma once

#include <QDialog>

class AuthService;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPushButton;

// 登录和注册共用一个对话框，m_isRegister 控制“昵称 / 确认密码”两行的显隐。
class LoginDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoginDialog(AuthService* auth, QWidget* parent = nullptr);

private:
    void setRegisterMode(bool on);
    void submit();

    AuthService*  m_auth;
    bool          m_isRegister = false;
    QFormLayout*  m_form     = nullptr;
    QLineEdit*    m_usernameEdit = nullptr;
    QLineEdit*    m_nicknameEdit = nullptr;
    QLineEdit*    m_pwdEdit      = nullptr;
    QLineEdit*    m_pwd2Edit     = nullptr;
    QLabel*       m_errorLabel   = nullptr;
    QPushButton*  m_submitButton = nullptr;
    QPushButton*  m_switchButton = nullptr;
};
