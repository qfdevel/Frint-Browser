#ifndef FRINT_STORAGE_PARTITION_H
#define FRINT_STORAGE_PARTITION_H

#include <QString>
#include <QUrl>

namespace Frint {

class StoragePartition {
public:
    static StoragePartition &instance();

    // Get a partition ID for a given origin
    // Each origin gets its own storage sandbox
    QString partitionIdForOrigin(const QUrl &url) const;

    // Get the storage path for a given origin
    QString storagePathForOrigin(const QUrl &url) const;

    // Clear all storage partitions
    void clearAll();

    // Clear storage for a specific origin
    void clearForOrigin(const QUrl &url);

    // Set the base storage directory
    void setBasePath(const QString &path);
    QString basePath() const;

private:
    StoragePartition();
    QString m_basePath;
};

} // namespace Frint

#endif // FRINT_STORAGE_PARTITION_H
