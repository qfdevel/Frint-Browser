#ifndef FRINT_READING_LIST_H
#define FRINT_READING_LIST_H

#include <QObject>
#include <QString>
#include <QList>
#include <QUrl>
#include <QDateTime>
#include <QJsonArray>

namespace Frint {

struct ReadingListItem {
    QUrl url;
    QString title;
    QString excerpt;
    QDateTime added;
    bool read = false;
};

class ReadingListManager : public QObject {
    Q_OBJECT

public:
    static ReadingListManager &instance();

    void addItem(const QUrl &url, const QString &title, const QString &excerpt = QString());
    void removeItem(const QUrl &url);
    void toggleRead(const QUrl &url);
    QList<ReadingListItem> items() const;
    QList<ReadingListItem> unreadItems() const;
    QList<ReadingListItem> readItems() const;
    bool hasItem(const QUrl &url) const;
    int count() const;

signals:
    void listChanged();

private:
    ReadingListManager();
    ~ReadingListManager() = default;
    ReadingListManager(const ReadingListManager &) = delete;
    ReadingListManager &operator=(const ReadingListManager &) = delete;

    void save();
    void load();
    QString storagePath() const;

    QList<ReadingListItem> m_items;
};

} // namespace Frint

#endif // FRINT_READING_LIST_H
