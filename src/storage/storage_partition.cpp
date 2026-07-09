#include "storage/storage_partition.h"
#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>

namespace Frint {

StoragePartition &StoragePartition::instance()
{
    static StoragePartition s_instance;
    return s_instance;
}

StoragePartition::StoragePartition()
    : m_basePath(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                 + "/partitions")
{
}

void StoragePartition::setBasePath(const QString &path)
{
    m_basePath = path;
}

QString StoragePartition::basePath() const
{
    return m_basePath;
}

QString StoragePartition::partitionIdForOrigin(const QUrl &url) const
{
    if (!url.isValid()) {
        return "null-origin";
    }

    // Create a unique ID based on scheme + host + port
    QString originKey = url.scheme() + "://" + url.host();
    if (url.port() > 0) {
        originKey += ":" + QString::number(url.port());
    }

    // Hash the origin to create a safe directory name
    QByteArray hash = QCryptographicHash::hash(
        originKey.toUtf8(), QCryptographicHash::Sha256);
    return hash.toHex().left(32);
}

QString StoragePartition::storagePathForOrigin(const QUrl &url) const
{
    QString partitionId = partitionIdForOrigin(url);
    QString path = m_basePath + "/" + partitionId;

    // Ensure the directory exists
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    return path;
}

void StoragePartition::clearAll()
{
    QDir dir(m_basePath);
    if (dir.exists()) {
        dir.removeRecursively();
    }
}

void StoragePartition::clearForOrigin(const QUrl &url)
{
    QString path = storagePathForOrigin(url);
    QDir dir(path);
    if (dir.exists()) {
        dir.removeRecursively();
    }
}

} // namespace Frint
