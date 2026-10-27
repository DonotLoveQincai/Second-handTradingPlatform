#include "services/import_service.h"

#include <QFile>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringConverter>
#include <QStringDecoder>
#include <QVariant>

#include "core/common/constants.h"
#include "core/entity/product.h"
#include "core/repository/i_product_repository.h"
#include "data/db_manager.h"

namespace {

// 把一个 CSV 行切成字段，处理引号和 "" 转义
QStringList parseCsvLine(const QString& line)
{
    QStringList out;
    QString field;
    bool inQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line.at(i);
        if (inQuotes) {
            if (c == QLatin1Char('"')) {
                if (i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
                    field += QLatin1Char('"');    // "" → 一个引号
                    ++i;
                } else {
                    inQuotes = false;             // 引号结束
                }
            } else {
                field += c;
            }
        } else {
            if (c == QLatin1Char('"')) {
                inQuotes = true;
            } else if (c == QLatin1Char(',')) {
                out << field;
                field.clear();
            } else {
                field += c;
            }
        }
    }
    out << field;                                  // 最后一个字段
    return out;
}

// 先按 UTF-8 解，失败再按系统编码（中文 Windows = GBK）解
QString decodeBytes(const QByteArray& bytes)
{
    QStringDecoder utf8(QStringConverter::Utf8);
    QString text = utf8.decode(bytes);
    if (utf8.hasError()) {
        QStringDecoder sys(QStringConverter::System);
        text = sys.decode(bytes);
    }
    if (text.startsWith(QChar(0xFEFF)))            // 去掉 BOM
        text.remove(0, 1);
    return text;
}

// 分类名（支持"数码电子/手机"这种父/子写法）→ id，找不到返回 -1
int findCategoryId(QSqlDatabase& db, const QString& name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty())
        return -1;

    const int slash = trimmed.indexOf(QLatin1Char('/'));
    const QString child  = slash >= 0 ? trimmed.mid(slash + 1).trimmed() : trimmed;
    const QString parent = slash >= 0 ? trimmed.left(slash).trimmed() : QString();

    QSqlQuery q(db);
    if (parent.isEmpty()) {
        q.prepare("SELECT id FROM categories WHERE name = ?");
        q.addBindValue(child);
    } else {
        q.prepare("SELECT c.id FROM categories c "
                  "JOIN categories p ON p.id = c.parent_id "
                  "WHERE c.name = ? AND p.name = ?");
        q.addBindValue(child);
        q.addBindValue(parent);
    }
    if (q.exec() && q.next())
        return q.value(0).toInt();
    return -1;
}

} // namespace

ImportService::ImportService(IProductRepository* productRepo, IUserRepository* userRepo,
                             QObject* parent)
    : QObject(parent)
    , m_productRepo(productRepo)
    , m_userRepo(userRepo)
{
}

ImportReport ImportService::importCsv(const QString& filePath, int defaultSellerId)
{
    ImportReport report;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        report.errors << QStringLiteral("无法打开文件：%1").arg(filePath);
        return report;
    }
    const QString text = decodeBytes(f.readAll());
    f.close();

    QStringList lines = text.split(QRegularExpression(QStringLiteral("\r\n|\n|\r")),
                                   Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        report.errors << QStringLiteral("文件是空的");
        return report;
    }

    // 第一行必须是表头
    const QStringList header = parseCsvLine(lines.first());
    QStringList expect = { "title", "category", "price", "condition_level", "description" };
    QStringList actual;
    for (const QString& h : header)
        actual << h.trimmed().toLower();
    if (actual.size() < 4 || actual.mid(0, 4) != expect.mid(0, 4)) {
        report.errors << QStringLiteral("表头不对，应该以 "
                                        "title,category,price,condition_level 开头，实际是：%1")
                             .arg(lines.first());
        return report;
    }
    const bool hasDescription = actual.size() >= 5 && actual.at(4) == QStringLiteral("description");

    // 逐行校验，全部先攒起来，不落库
    QVector<Product> rows;
    QSqlDatabase& db = DbManager::db();

    for (int i = 1; i < lines.size(); ++i) {
        const int lineNo = i + 1;
        ++report.total;
        const QStringList cols = parseCsvLine(lines.at(i));

        if (cols.size() < 4) {
            report.errors << QStringLiteral("第 %1 行：列数不足（至少 4 列）").arg(lineNo);
            continue;
        }

        const QString title = cols.at(0).trimmed();
        const QString cat   = cols.at(1).trimmed();
        const QString priceStr = cols.at(2).trimmed();
        const QString condStr  = cols.at(3).trimmed();
        const QString desc  = hasDescription && cols.size() >= 5 ? cols.at(4).trimmed() : QString();

        if (title.isEmpty()) {
            report.errors << QStringLiteral("第 %1 行：标题不能为空").arg(lineNo);
            continue;
        }
        if (title.size() > 60) {
            report.errors << QStringLiteral("第 %1 行：标题超过 60 字").arg(lineNo);
            continue;
        }

        bool priceOk = false;
        const double price = priceStr.toDouble(&priceOk);
        if (!priceOk || price <= 0) {
            report.errors << QStringLiteral("第 %1 行：价格不是有效数字（%2）").arg(lineNo).arg(priceStr);
            continue;
        }

        int cond = 9;
        if (!condStr.isEmpty()) {
            bool condOk = false;
            cond = condStr.toInt(&condOk);
            if (!condOk || cond < 1 || cond > 10) {
                report.errors << QStringLiteral("第 %1 行：成色必须是 1~10 的整数").arg(lineNo);
                continue;
            }
        }

        const int catId = findCategoryId(db, cat);
        if (catId <= 0) {
            report.errors << QStringLiteral("第 %1 行：分类\"%2\"不存在").arg(lineNo).arg(cat);
            continue;
        }

        Product p;
        p.sellerId       = defaultSellerId;
        p.categoryId     = catId;
        p.title          = title;
        p.description    = desc;
        p.price          = price;
        p.conditionLevel = cond;
        rows << p;
    }

    if (!report.errors.isEmpty()) {
        report.succeeded = 0;
        return report;                     // 有错就一条不导
    }

    // 全部合法 → 整批一个事务
    if (!db.transaction()) {
        report.errors << QStringLiteral("开启事务失败");
        return report;
    }
    for (int i = 0; i < rows.size(); ++i) {
        auto r = m_productRepo->insert(rows[i]);
        if (!r.ok) {
            db.rollback();
            report.errors << QStringLiteral("第 %1 条写入失败：%2").arg(i + 1).arg(r.message);
            report.succeeded = 0;
            return report;
        }
    }
    if (!db.commit()) {
        db.rollback();
        report.errors << QStringLiteral("提交事务失败");
        report.succeeded = 0;
        return report;
    }

    report.succeeded = rows.size();
    return report;
}
