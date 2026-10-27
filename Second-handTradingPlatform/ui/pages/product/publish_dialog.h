#pragma once
#include <QDialog>

#include "core/entity/product.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class ProductService;

class PublishDialog : public QDialog
{
    Q_OBJECT
public:
    PublishDialog(ProductService* service, int sellerId, QWidget* parent = nullptr);

    void setProduct(const Product& product);

private slots:
    void onSubmitClicked();

private:
    ProductService* m_service = nullptr;
    int m_sellerId = 0;
    int m_editId = 0;

    QLineEdit* m_title = nullptr;
    QComboBox* m_cat = nullptr;
    QDoubleSpinBox* m_price = nullptr;
    QSpinBox* m_cond = nullptr;
    QPlainTextEdit* m_desc = nullptr;
    QLabel* m_err = nullptr;
    QPushButton* m_ok = nullptr;
};
