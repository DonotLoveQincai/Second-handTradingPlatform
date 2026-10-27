#pragma once
#include <QDialog>
#include <QVector>

#include "core/entity/product.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class ProductService;

// 发布商品对话框 [A]：发布和“修改信息”共用这一个框（字段完全一样）。
class PublishDialog : public QDialog
{
    Q_OBJECT
public:
    // 发布模式
    PublishDialog(ProductService* service, int sellerId, QWidget* parent = nullptr);

    // 修改模式：把已有商品填进表单（有 id = 编辑，0 = 新建）
    void setProduct(const Product& product);
    int productId() const { return m_editingId; }

private slots:
    void onSubmitClicked();

private:
    void buildUi();
    void loadCategories();
    bool collect(Product& out, QString* error) const;

    ProductService* m_service = nullptr;
    int m_sellerId = 0;
    int m_editingId = 0;              // 0 = 新建，>0 = 修改
    Product m_original;

    QLineEdit* m_titleEdit = nullptr;
    QComboBox* m_categoryBox = nullptr;
    QDoubleSpinBox* m_priceSpin = nullptr;
    QSpinBox* m_conditionSpin = nullptr;
    QPlainTextEdit* m_descEdit = nullptr;
    QLabel* m_errorLabel = nullptr;
};
