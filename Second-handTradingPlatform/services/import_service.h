#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class IProductRepository;
class IUserRepository;

struct ImportReport
{
    int         total     = 0;
    int         succeeded = 0;
    QStringList errors;          // "第 3 行：价格不是有效数字"
};

// CSV 批量导入：只要有 1 行不合法，一条都不入库
class ImportService : public QObject
{
    Q_OBJECT
public:
    ImportService(IProductRepository* productRepo, IUserRepository* userRepo,
                  QObject* parent = nullptr);

    ImportReport importCsv(const QString& filePath, int defaultSellerId);

private:
    IProductRepository* m_productRepo;
    IUserRepository*    m_userRepo;
};
