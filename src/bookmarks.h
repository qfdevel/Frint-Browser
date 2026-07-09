#ifndef FRINT_BOOKMARKS_H
#define FRINT_BOOKMARKS_H

#include <QObject>
#include <QString>
#include <QList>
#include <QUrl>
#include <QJsonArray>
#include <QJsonObject>

namespace Frint {

struct Bookmark {
    QString title;
    QUrl url;
    QString folder;    // empty = root
    QDateTime added;
    QString iconPath;
};

class BookmarkManager : public QObject {
    Q_OBJECT

public:
    static BookmarkManager &instance();

    void addBookmark(const QString &title, const QUrl &url, const QString &folder = QString());
    void removeBookmark(const QUrl &url);
    bool hasBookmark(const QUrl &url) const;
    QList<Bookmark> bookmarks(const QString &folder = QString()) const;
    QStringList folders() const;
    void addFolder(const QString &name);
    void removeFolder(const QString &name);
    void renameFolder(const QString &oldName, const QString &newName);

    void exportToHtml(const QString &path);
    void importFromHtml(const QString &path);
    void exportToJson(const QString &path);
    void importFromJson(const QString &path);

    int count() const;

signals:
    void bookmarksChanged();

private:
    BookmarkManager();
    ~BookmarkManager() = default;
    BookmarkManager(const BookmarkManager &) = delete;
    BookmarkManager &operator=(const BookmarkManager &) = delete;

    void save();
    void load();
    QString storagePath() const;

    QList<Bookmark> m_bookmarks;
    QStringList m_folders;
};

} // namespace Frint

#endif // FRINT_BOOKMARKS_H
