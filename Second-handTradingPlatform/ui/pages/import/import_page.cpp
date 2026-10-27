#include "ui/pages/import/import_page.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "core/session.h"
#include "services/import_service.h"

ImportPage::ImportPage(ImportService* service, QWidget* parent)
    : QWidget(parent)
    , m_service(service)
{
    auto* root = new QVBoxLayout(this);

    auto* tip = new QLabel(QStringLiteral(
        "CSV 第一行必须是表头：title,category,price,condition_level,description\n"
        "卖家就是当前登录用户。任何一行出错，整批都不会导入。"), this);
    root->addWidget(tip);

    auto* row = new QHBoxLayout();
    m_pathEdit  = new QLineEdit(this);
    m_pathEdit->setReadOnly(true);
    m_pathEdit->setPlaceholderText(QStringLiteral("还没有选择文件"));
    m_browseBtn = new QPushButton(QStringLiteral("选择 CSV 文件…"), this);
    m_importBtn = new QPushButton(QStringLiteral("开始导入"), this);
    m_importBtn->setEnabled(false);
    row->addWidget(m_pathEdit, 1);
    row->addWidget(m_browseBtn);
    row->addWidget(m_importBtn);
    root->addLayout(row);

    m_resultEdit = new QPlainTextEdit(this);
    m_resultEdit->setReadOnly(true);
    root->addWidget(m_resultEdit, 1);

    connect(m_browseBtn, &QPushButton::clicked, this, &ImportPage::onBrowse);
    connect(m_importBtn, &QPushButton::clicked, this, &ImportPage::onImport);
}

void ImportPage::onBrowse()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择 CSV 文件"), QString(),
        QStringLiteral("CSV 文件 (*.csv);;所有文件 (*.*)"));
    if (path.isEmpty())
        return;

    m_pathEdit->setText(path);
    m_importBtn->setEnabled(true);
    m_resultEdit->setPlainText(QStringLiteral("已选择：") + path);
}

void ImportPage::onImport()
{
    if (!Session::instance().isLoggedIn()) {
        m_resultEdit->setPlainText(QStringLiteral("请先登录再导入"));
        return;
    }
    if (!m_service) {
        m_resultEdit->setPlainText(QStringLiteral("导入服务不可用"));
        return;
    }

    const ImportReport r = m_service->importCsv(m_pathEdit->text(),
                                                Session::instance().currentUser().id);

    QString out = QStringLiteral("共 %1 行，成功 %2 行\n").arg(r.total).arg(r.succeeded);
    if (!r.errors.isEmpty()) {
        out += QStringLiteral("\n错误：\n");
        for (const QString& e : r.errors)
            out += QStringLiteral("· ") + e + QLatin1Char('\n');
    }
    m_resultEdit->setPlainText(out);
}
