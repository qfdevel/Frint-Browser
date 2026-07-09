#ifndef FRINT_STORAGE_PARTITION_H
#define FRINT_STORAGE_PARTITION_H

#include <QString>
#include <QUrl>

namespace Frint {

class StoragePartition {
public:
    static StoragePartition &instance();

    // Get a unique partition ID for a URL's origin
    QString partitionIdForOrigin(const QUrl &url) const;

    // Get the filesystem path for an origin's storage
    QString storagePathForOrigin(const QUrl &url) const;

    // Ensure the partition directory exists
    QString ensurePartition(const QUrl &url);

    // Clear all storage partitions
    void clearAll();

    // Clear storage for a specific origin
    void clearForOrigin(const QUrl &url);

    // Set/get base path (default: AppLocalDataLocation/frint/partitions)
    void setBasePath(const QString &path);
    QString basePath() const;

private:
    StoragePartition();
    StoragePartition(const StoragePartition &) = delete;
    StoragePartition &operator=(const StoragePartition &) = delete;

    QString m_basePath;
};

} // namespace Frint

#endif // FRINT_STORAGE_PARTITION_H
