#include "ui/pages/product/publish_dialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "core/common/constants.h"
#include "services/product_service.h"

PublishDialog::PublishDialog(ProductService* service, int sellerId, QWidget* parent)
    : QDialog(parent)
    , m_service(service)
    , m_sellerId(sellerId)
{
    setWindowTitle(QStringLiteral("发布商品"));
    buildUi();
    loadCategories();
}

void PublishDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);

    auto* form = new QFormLayout;

    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setMaxLength(60);
    m_titleEdit->setPlaceholderText(QStringLiteral("例如：九成新 iPhone 13"));
    form->addRow(QStringLiteral("标题"), m_titleEdit);

    m_categoryBox = new QComboBox(this);
    form->addRow(QStringLiteral("分类"), m_categoryBox);

    m_priceSpin = new QDoubleSpinBox(this);
    m_priceSpin->setRange(0.01, 999999);
    m_priceSpin->setDecimals(2);
    m_priceSpin->setSuffix(QStringLiteral(" 元"));
    form->addRow(QStringLiteral("价格"), m_priceSpin);

    m_conditionSpin = new QSpinBox(this);
    m_conditionSpin->setRange(1, 10);
    m_conditionSpin->setValue(9);
    m_conditionSpin->setSuffix(QStringLiteral(" 成新（10 = 全新）"));
    form->addRow(QStringLiteral("成色"), m_conditionSpin);

    m_descEdit = new QPlainTextEdit(this);
    m_descEdit->setPlaceholderText(QStringLiteral("可选：型号、磨损、附件…"));
    m_descEdit->setFixedHeight(90);
    form->addRow(QStringLiteral("描述"), m_descEdit);

    root->addLayout(form);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet(QStringLiteral("color:#c0392b;"));
    m_errorLabel->setWordWrap(true);
    root->addWidget(m_errorLabel);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("提交"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    root->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &PublishDialog::onSubmitClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &PublishDialog::reject);
}

void PublishDialog::loadCategories()
{
    m_categoryBox->clear();
    const QVector<CategoryNode> categories = m_service->categories();
    for (const CategoryNode& parent : categories) {
        if (parent.parentId != 0)
            continue;
        for (const CategoryNode& child : categories) {
            if (child.parentId != parent.id)
                continue;
            m_categoryBox->addItem(QStringLiteral("%1 / %2").arg(parent.name, child.name), child.id);
        }
    }
    // 没有子分类的顶级分类也要能选（图书教材、家居日用…）
    for (const CategoryNode& parent : categories) {
        if (parent.parentId != 0)
            continue;
        bool hasChild = false;
        for (const CategoryNode& child : categories)
            if (child.parentId == parent.id)
                hasChild = true;
        if (!hasChild)
            m_categoryBox->addItem(parent.name, parent.id);
    }
}

void PublishDialog::setProduct(const Product& product)
{
    m_original = product;
    m_editingId = product.id;
    setWindowTitle(QStringLiteral("修改商品"));

    m_titleEdit->setText(product.title);
    const int index = m_categoryBox->findData(product.categoryId);
    if (index >= 0)
        m_categoryBox->setCurrentIndex(index);
    m_priceSpin->setValue(product.price);
    m_conditionSpin->setValue(product.conditionLevel);
    m_descEdit->setPlainText(product.description);
}

bool PublishDialog::collect(Product& out, QString* error) const
{
    const QString title = m_titleEdit->text().trimmed();
    if (title.isEmpty()) {
        *error = QStringLiteral("标题不能为空");
        return false;
    }
    if (title.size() > 60) {
        *error = QStringLiteral("标题不能超过 60 个字");
        return false;
    }
    if (m_categoryBox->currentData().toInt() <= 0) {
        *error = QStringLiteral("请选择分类");
        return false;
    }
    if (m_priceSpin->value() <= 0) {
        *error = QStringLiteral("价格必须大于 0");
        return false;
    }

    out = m_original;                          // 保留 id / status / 时间等字段
    out.title = title;
    out.description = m_descEdit->toPlainText().trimmed();
    out.categoryId = m_categoryBox->currentData().toInt();
    out.price = m_priceSpin->value();
    out.conditionLevel = m_conditionSpin->value();
    out.sellerId = m_sellerId;
    return true;
}

void PublishDialog::onSubmitClicked()
{
    Product product;
    QString error;
    if (!collect(product, &error)) {
        m_errorLabel->setText(error);
        return;
    }

    const OpResult result = (m_editingId > 0)
                                ? m_service->updateInfo(product, m_sellerId)
                                : m_service->publish(product, m_sellerId);

    if (!result.ok) {
        m_errorLabel->setText(result.message);
        return;
    }
    accept();
}
