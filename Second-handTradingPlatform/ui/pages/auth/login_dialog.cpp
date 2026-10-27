#include "ui/pages/auth/login_dialog.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "services/auth_service.h"

LoginDialog::LoginDialog(AuthService* auth, QWidget* parent)
    : QDialog(parent)
    , m_auth(auth)
{
    setWindowTitle(QStringLiteral("登录"));
    setModal(true);
    setMinimumWidth(380);

    auto* root = new QVBoxLayout(this);

    m_form = new QFormLayout();
    m_form->setLabelAlignment(Qt::AlignRight);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText(QStringLiteral("3 个字符以上"));
    m_form->addRow(QStringLiteral("用户名"), m_usernameEdit);

    m_nicknameEdit = new QLineEdit(this);
    m_nicknameEdit->setPlaceholderText(QStringLiteral("不填就用用户名"));
    m_form->addRow(QStringLiteral("昵称"), m_nicknameEdit);

    m_pwdEdit = new QLineEdit(this);
    m_pwdEdit->setEchoMode(QLineEdit::Password);
    m_pwdEdit->setPlaceholderText(QStringLiteral("6 位以上"));
    m_form->addRow(QStringLiteral("密码"), m_pwdEdit);

    m_pwd2Edit = new QLineEdit(this);
    m_pwd2Edit->setEchoMode(QLineEdit::Password);
    m_form->addRow(QStringLiteral("确认密码"), m_pwd2Edit);

    root->addLayout(m_form);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet(QStringLiteral("color:#c0392b;"));
    m_errorLabel->setWordWrap(true);
    root->addWidget(m_errorLabel);

    auto* buttons = new QVBoxLayout();
    m_submitButton = new QPushButton(QStringLiteral("登录"), this);
    m_submitButton->setDefault(true);
    m_switchButton = new QPushButton(QStringLiteral("没有账号？去注册"), this);
    m_switchButton->setFlat(true);
    buttons->addWidget(m_submitButton);
    buttons->addWidget(m_switchButton);
    root->addLayout(buttons);

    connect(m_submitButton, &QPushButton::clicked, this, &LoginDialog::submit);
    connect(m_switchButton, &QPushButton::clicked, this,
        [this] { setRegisterMode(!m_isRegister); });
    connect(m_usernameEdit, &QLineEdit::returnPressed, this, &LoginDialog::submit);
    connect(m_nicknameEdit, &QLineEdit::returnPressed, this, &LoginDialog::submit);
    connect(m_pwdEdit, &QLineEdit::returnPressed, this, &LoginDialog::submit);
    connect(m_pwd2Edit, &QLineEdit::returnPressed, this, &LoginDialog::submit);

    setRegisterMode(false);
}

void LoginDialog::setRegisterMode(bool on)
{
    m_isRegister = on;

    m_form->setRowVisible(m_nicknameEdit, on);
    m_form->setRowVisible(m_pwd2Edit, on);

    setWindowTitle(on ? QStringLiteral("注册") : QStringLiteral("登录"));
    m_submitButton->setText(on ? QStringLiteral("注册并登录") : QStringLiteral("登录"));
    m_switchButton->setText(on ? QStringLiteral("已有账号？去登录")
                               : QStringLiteral("没有账号？去注册"));
    m_errorLabel->clear();
    (on ? m_nicknameEdit : m_usernameEdit)->setFocus();
}

void LoginDialog::submit()
{
    m_errorLabel->clear();

    const QString name = m_usernameEdit->text();
    const QString pwd  = m_pwdEdit->text();

    const OpResult r = m_isRegister
        ? m_auth->registerUser(name, m_nicknameEdit->text(), pwd, m_pwd2Edit->text())
        : m_auth->login(name, pwd);

    if (!r.ok) {
        m_errorLabel->setText(r.message);
        return;
    }
    accept();
}
