#include "bookmarks.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QDebug>
#include <QTextStream>

namespace Frint {

BookmarkManager &BookmarkManager::instance()
{
    static BookmarkManager s_instance;
    return s_instance;
}

BookmarkManager::BookmarkManager()
{
    load();
}

QString BookmarkManager::storagePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                  + "/Frint";
    QDir().mkpath(dir);
    return dir + "/bookmarks.json";
}

void BookmarkManager::save()
{
    QJsonObject root;
    QJsonArray foldersArr;
    for (const auto &f : m_folders)
        foldersArr.append(f);
    root["folders"] = foldersArr;

    QJsonArray bookmarksArr;
    for (const auto &b : m_bookmarks) {
        QJsonObject obj;
        obj["title"] = b.title;
        obj["url"] = b.url.toString();
        obj["folder"] = b.folder;
        obj["added"] = b.added.toString(Qt::ISODate);
        bookmarksArr.append(obj);
    }
    root["bookmarks"] = bookmarksArr;

    QFile file(storagePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void BookmarkManager::load()
{
    m_bookmarks.clear();
    m_folders.clear();

    QFile file(storagePath());
    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) return;
    QJsonObject root = doc.object();

    QJsonArray foldersArr = root["folders"].toArray();
    for (const auto &f : foldersArr)
        m_folders.append(f.toString());

    QJsonArray bookmarksArr = root["bookmarks"].toArray();
    for (const auto &val : bookmarksArr) {
        QJsonObject obj = val.toObject();
        Bookmark b;
        b.title = obj["title"].toString();
        b.url = QUrl(obj["url"].toString());
        b.folder = obj["folder"].toString();
        b.added = QDateTime::fromString(obj["added"].toString(), Qt::ISODate);
        if (b.url.isValid())
            m_bookmarks.append(b);
    }

    // Ensure at least "Default" folder
    if (!m_folders.contains("Default"))
        m_folders.prepend("Default");
}

void BookmarkManager::addBookmark(const QString &title, const QUrl &url, const QString &folder)
{
    if (!url.isValid()) return;
    // Prevent duplicates
    for (const auto &b : m_bookmarks) {
        if (b.url == url) return;
    }
    Bookmark b;
    b.title = title.isEmpty() ? url.toString() : title;
    b.url = url;
    b.folder = folder.isEmpty() ? "Default" : folder;
    b.added = QDateTime::currentDateTime();
    m_bookmarks.append(b);
    save();
    emit bookmarksChanged();
}

void BookmarkManager::removeBookmark(const QUrl &url)
{
    m_bookmarks.erase(std::remove_if(m_bookmarks.begin(), m_bookmarks.end(),
        [&url](const Bookmark &b) { return b.url == url; }),
        m_bookmarks.end());
    save();
    emit bookmarksChanged();
}

bool BookmarkManager::hasBookmark(const QUrl &url) const
{
    return std::any_of(m_bookmarks.begin(), m_bookmarks.end(),
        [&url](const Bookmark &b) { return b.url == url; });
}

QList<Bookmark> BookmarkManager::bookmarks(const QString &folder) const
{
    if (folder.isEmpty())
        return m_bookmarks;
    QList<Bookmark> result;
    for (const auto &b : m_bookmarks) {
        if (b.folder == folder)
            result.append(b);
    }
    return result;
}

QStringList BookmarkManager::folders() const { return m_folders; }

void BookmarkManager::addFolder(const QString &name)
{
    if (!m_folders.contains(name)) {
        m_folders.append(name);
        save();
        emit bookmarksChanged();
    }
}

void BookmarkManager::removeFolder(const QString &name)
{
    if (name == "Default") return;
    m_folders.removeAll(name);
    // Move bookmarks from deleted folder to Default
    for (auto &b : m_bookmarks) {
        if (b.folder == name)
            b.folder = "Default";
    }
    save();
    emit bookmarksChanged();
}

void BookmarkManager::renameFolder(const QString &oldName, const QString &newName)
{
    if (oldName == "Default" || newName.isEmpty()) return;
    int idx = m_folders.indexOf(oldName);
    if (idx >= 0) m_folders[idx] = newName;
    for (auto &b : m_bookmarks) {
        if (b.folder == oldName)
            b.folder = newName;
    }
    save();
    emit bookmarksChanged();
}

int BookmarkManager::count() const { return m_bookmarks.size(); }

void BookmarkManager::exportToHtml(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    out << "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n"
        << "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">\n"
        << "<TITLE>Bookmarks</TITLE>\n"
        << "<H1>Bookmarks</H1>\n<DL><p>\n";
    for (const auto &f : m_folders) {
        out << "    <DT><H3>" << f.toHtmlEscaped() << "</H3>\n    <DL><p>\n";
        for (const auto &b : m_bookmarks) {
            if (b.folder != f) continue;
            out << "        <DT><A HREF=\"" << b.url.toString().toHtmlEscaped() << "\">"
                << b.title.toHtmlEscaped() << "</A>\n";
        }
        out << "    </DL><p>\n";
    }
    out << "</DL><p>\n";
    file.close();
}

void BookmarkManager::importFromHtml(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    QString folderName = "Imported";
    addFolder(folderName);
    while (!in.atEnd()) {
        QString line = in.readLine();
        // Match <A HREF="...">title</A>
        int hrefPos = line.indexOf("HREF=\"");
        if (hrefPos < 0) continue;
        hrefPos += 6;
        int hrefEnd = line.indexOf("\"", hrefPos);
        if (hrefEnd < 0) continue;
        QString urlStr = line.mid(hrefPos, hrefEnd - hrefPos);
        int titleStart = line.indexOf(">", hrefEnd);
        int titleEnd = line.indexOf("</A>", titleStart);
        if (titleStart > 0 && titleEnd > 0) {
            QString title = line.mid(titleStart + 1, titleEnd - titleStart - 1);
            addBookmark(title, QUrl(urlStr), folderName);
        }
    }
    file.close();
}

void BookmarkManager::exportToJson(const QString &path)
{
    save();
    QFile::copy(storagePath(), path);
}

void BookmarkManager::importFromJson(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return;

    QJsonObject root = doc.object();
    QJsonArray bookmarksArr = root["bookmarks"].toArray();
    for (const auto &val : bookmarksArr) {
        QJsonObject obj = val.toObject();
        addBookmark(obj["title"].toString(),
                    QUrl(obj["url"].toString()),
                    obj["folder"].toString());
    }
}

} // namespace Frint
