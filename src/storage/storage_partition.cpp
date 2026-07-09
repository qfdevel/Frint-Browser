#include "storage/storage_partition.h"
#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

namespace Frint {

StoragePartition &StoragePartition::instance()
{
    static StoragePartition s_instance;
    return s_instance;
}

StoragePartition::StoragePartition()
    : m_basePath(QStandardPaths::writableLocation(
                     QStandardPaths::AppLocalDataLocation)
                 + "/frint/partitions")
{
}

void StoragePartition::setBasePath(const QString &path)
{
    m_basePath = path;
    qDebug() << "[Frint StoragePartition] Base path:" << m_basePath;
}

QString StoragePartition::basePath() const
{
    return m_basePath;
}

QString StoragePartition::partitionIdForOrigin(const QUrl &url) const
{
    if (!url.isValid() || url.host().isEmpty()) {
        return "null-origin";
    }

    // Build origin key: scheme + "://" + host [+ ":" + port]
    int defaultPort = (url.scheme() == "https") ? 443
                    : (url.scheme() == "http")  ? 80 : -1;
    QString originKey = url.scheme() + "://" + url.host();
    if (url.port() > 0 && url.port() != defaultPort) {
        originKey += ":" + QString::number(url.port());
    }

    // SHA-256 hash for safe directory name
    QByteArray hash = QCryptographicHash::hash(
        originKey.toUtf8(), QCryptographicHash::Sha256);

    // Use first 16 hex chars as partition ID
    return hash.toHex().left(16);
}

QString StoragePartition::storagePathForOrigin(const QUrl &url) const
{
    return m_basePath + "/" + partitionIdForOrigin(url);
}

QString StoragePartition::ensurePartition(const QUrl &url)
{
    QString path = storagePathForOrigin(url);
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
        qDebug() << "[Frint StoragePartition] Created partition:" << path
                 << "for origin:" << url.scheme() + "://" + url.host();
    }
    return path;
}

void StoragePartition::clearAll()
{
    QDir dir(m_basePath);
    if (dir.exists()) {
        if (dir.removeRecursively()) {
            qDebug() << "[Frint StoragePartition] All partitions cleared";
        }
    }
}

void StoragePartition::clearForOrigin(const QUrl &url)
{
    QString path = storagePathForOrigin(url);
    QDir dir(path);
    if (dir.exists()) {
        if (dir.removeRecursively()) {
            qDebug() << "[Frint StoragePartition] Cleared partition for:"
                     << url.scheme() + "://" + url.host();
        }
    }
}

} // namespace Frint
