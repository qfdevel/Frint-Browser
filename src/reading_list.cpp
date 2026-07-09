#include "reading_list.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

namespace Frint {

ReadingListManager &ReadingListManager::instance()
{
    static ReadingListManager s_instance;
    return s_instance;
}

ReadingListManager::ReadingListManager()
{
    load();
}

QString ReadingListManager::storagePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                  + "/Frint";
    QDir().mkpath(dir);
    return dir + "/readinglist.json";
}

void ReadingListManager::save()
{
    QJsonArray arr;
    for (const auto &item : m_items) {
        QJsonObject obj;
        obj["url"] = item.url.toString();
        obj["title"] = item.title;
        obj["excerpt"] = item.excerpt;
        obj["added"] = item.added.toString(Qt::ISODate);
        obj["read"] = item.read;
        arr.append(obj);
    }
    QJsonObject root;
    root["items"] = arr;
    QFile file(storagePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void ReadingListManager::load()
{
    m_items.clear();
    QFile file(storagePath());
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return;

    QJsonArray arr = doc.object()["items"].toArray();
    for (const auto &val : arr) {
        QJsonObject obj = val.toObject();
        ReadingListItem item;
        item.url = QUrl(obj["url"].toString());
        item.title = obj["title"].toString();
        item.excerpt = obj["excerpt"].toString();
        item.added = QDateTime::fromString(obj["added"].toString(), Qt::ISODate);
        item.read = obj["read"].toBool();
        if (item.url.isValid())
            m_items.append(item);
    }
}

void ReadingListManager::addItem(const QUrl &url, const QString &title, const QString &excerpt)
{
    if (!url.isValid()) return;
    for (const auto &item : m_items) {
        if (item.url == url) return;
    }
    ReadingListItem item;
    item.url = url;
    item.title = title.isEmpty() ? url.toString() : title;
    item.excerpt = excerpt;
    item.added = QDateTime::currentDateTime();
    item.read = false;
    m_items.prepend(item);
    save();
    emit listChanged();
}

void ReadingListManager::removeItem(const QUrl &url)
{
    m_items.erase(std::remove_if(m_items.begin(), m_items.end(),
        [&url](const ReadingListItem &item) { return item.url == url; }),
        m_items.end());
    save();
    emit listChanged();
}

void ReadingListManager::toggleRead(const QUrl &url)
{
    for (auto &item : m_items) {
        if (item.url == url) {
            item.read = !item.read;
            break;
        }
    }
    save();
    emit listChanged();
}

QList<ReadingListItem> ReadingListManager::items() const { return m_items; }

QList<ReadingListItem> ReadingListManager::unreadItems() const
{
    QList<ReadingListItem> result;
    for (const auto &item : m_items) {
        if (!item.read) result.append(item);
    }
    return result;
}

QList<ReadingListItem> ReadingListManager::readItems() const
{
    QList<ReadingListItem> result;
    for (const auto &item : m_items) {
        if (item.read) result.append(item);
    }
    return result;
}

bool ReadingListManager::hasItem(const QUrl &url) const
{
    return std::any_of(m_items.begin(), m_items.end(),
        [&url](const ReadingListItem &item) { return item.url == url; });
}

int ReadingListManager::count() const { return m_items.size(); }

} // namespace Frint
