#pragma once

#include <QWidget>

class ImportService;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

// 批量导入页（主窗口第 5 个导航页）：选一个 CSV，交给 ImportService。
// 本页只负责选文件和显示结果，校验与入库规则全在 ImportService 里。
class ImportPage : public QWidget
{
    Q_OBJECT
public:
    explicit ImportPage(ImportService* service, QWidget* parent = nullptr);

public slots:
    void refresh() {}                 // 导入页没有列表要刷新，留空

private slots:
    void onBrowse();
    void onImport();

private:
    ImportService*  m_service = nullptr;
    QLineEdit*      m_pathEdit = nullptr;
    QPushButton*    m_browseBtn = nullptr;
    QPushButton*    m_importBtn = nullptr;
    QPlainTextEdit* m_resultEdit = nullptr;
};
