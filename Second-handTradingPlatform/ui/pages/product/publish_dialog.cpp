#include "ui/pages/product/publish_dialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "services/product_service.h"

PublishDialog::PublishDialog(ProductService* service, int sellerId, QWidget* parent)
    : QDialog(parent), m_service(service), m_sellerId(sellerId)
{
    setWindowTitle("发布商品");

    QVBoxLayout* root = new QVBoxLayout(this);
    QFormLayout* form = new QFormLayout;

    m_title = new QLineEdit(this);
    m_title->setMaxLength(60);
    m_title->setPlaceholderText("例如：九成新 iPhone 13");
    form->addRow("标题", m_title);

    m_cat = new QComboBox(this);
    form->addRow("分类", m_cat);

    m_price = new QDoubleSpinBox(this);
    m_price->setRange(0.01, 999999);
    m_price->setDecimals(2);
    m_price->setSuffix(" 元");
    form->addRow("价格", m_price);

    m_cond = new QSpinBox(this);
    m_cond->setRange(1, 10);
    m_cond->setValue(9);
    m_cond->setSuffix(" 成新（10 就是全新）");
    form->addRow("成色", m_cond);

    m_desc = new QPlainTextEdit(this);
    m_desc->setFixedHeight(90);
    m_desc->setPlaceholderText("型号、磨损、配件，随便写");
    form->addRow("描述", m_desc);

    root->addLayout(form);

    m_err = new QLabel(this);
    m_err->setStyleSheet("color:#c0392b;");
    m_err->setWordWrap(true);
    root->addWidget(m_err);

    QDialogButtonBox* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_ok = box->button(QDialogButtonBox::Ok);
    m_ok->setText("提交");
    box->button(QDialogButtonBox::Cancel)->setText("取消");
    root->addWidget(box);

    QVector<Category> cats = m_service->categories();
    for (int i = 0; i < cats.size(); i++) {
        if (cats[i].parentId != 0)
            continue;
        bool hasChild = false;
        for (int j = 0; j < cats.size(); j++) {
            if (cats[j].parentId == cats[i].id) {
                hasChild = true;
                m_cat->addItem(cats[i].name + " / " + cats[j].name, cats[j].id);
            }
        }
        if (!hasChild)
            m_cat->addItem(cats[i].name, cats[i].id);
    }

    connect(box, &QDialogButtonBox::accepted, this, &PublishDialog::onSubmitClicked);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void PublishDialog::setProduct(const Product& p)
{
    m_editId = p.id;
    setWindowTitle("修改商品");
    m_ok->setText("保存");

    m_title->setText(p.title);
    int idx = m_cat->findData(p.categoryId);
    if (idx >= 0)
        m_cat->setCurrentIndex(idx);
    m_price->setValue(p.price);
    m_cond->setValue(p.conditionLevel);
    m_desc->setPlainText(p.description);
}

void PublishDialog::onSubmitClicked()
{
    QString title = m_title->text().trimmed();
    if (title.isEmpty()) {
        m_err->setText("标题还没写");
        return;
    }
    if (title.size() > 60) {
        m_err->setText("标题太长了，60 个字以内");
        return;
    }
    int catId = m_cat->currentData().toInt();
    if (catId <= 0) {
        m_err->setText("还没选分类");
        return;
    }
    if (m_price->value() <= 0) {
        m_err->setText("价格得大于 0");
        return;
    }

    Product p;
    p.id = m_editId;
    p.title = title;
    p.categoryId = catId;
    p.price = m_price->value();
    p.conditionLevel = m_cond->value();
    p.description = m_desc->toPlainText().trimmed();
    p.sellerId = m_sellerId;

    OpResult r = (m_editId > 0) ? m_service->updateInfo(p, m_sellerId)
                                : m_service->publish(p, m_sellerId);
    if (!r.ok) {
        m_err->setText(r.message);
        return;
    }
    accept();
}
