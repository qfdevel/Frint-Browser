#include "history_manager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <algorithm>

namespace Frint {

HistoryManager &HistoryManager::instance()
{
    static HistoryManager s_instance;
    return s_instance;
}

HistoryManager::HistoryManager()
{
    load();
}

QString HistoryManager::storagePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                  + "/Frint";
    QDir().mkpath(dir);
    return dir + "/history.json";
}

void HistoryManager::save()
{
    QJsonArray arr;
    for (const auto &e : m_entries) {
        QJsonObject obj;
        obj["url"] = e.url.toString();
        obj["title"] = e.title;
        obj["time"] = e.visitTime.toString(Qt::ISODate);
        obj["count"] = e.visitCount;
        arr.append(obj);
    }
    QFile file(storagePath());
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject root;
        root["entries"] = arr;
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void HistoryManager::load()
{
    m_entries.clear();
    QFile file(storagePath());
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return;

    QJsonArray arr = doc.object()["entries"].toArray();
    for (const auto &val : arr) {
        QJsonObject obj = val.toObject();
        HistoryEntry e;
        e.url = QUrl(obj["url"].toString());
        e.title = obj["title"].toString();
        e.visitTime = QDateTime::fromString(obj["time"].toString(), Qt::ISODate);
        e.visitCount = obj["count"].toInt(1);
        if (e.url.isValid())
            m_entries.append(e);
    }
}

void HistoryManager::addVisit(const QUrl &url, const QString &title)
{
    if (!url.isValid() || url.scheme() == "about" || url.scheme() == "data" || url.scheme() == "blob" || url.scheme() == "frint")
        return;

    // Find existing entry by linear search (a copy for safe manipulation)
    int foundIdx = -1;
    HistoryEntry existing;
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].url == url) {
            foundIdx = i;
            existing = m_entries[i];
            break;
        }
    }

    if (foundIdx >= 0) {
        // Update and move to front (safe copy, then remove, then prepend)
        existing.visitTime = QDateTime::currentDateTime();
        existing.visitCount++;
        if (!title.isEmpty())
            existing.title = title;
        m_entries.removeAt(foundIdx);
        m_entries.prepend(existing);
    } else {
        // New entry
        HistoryEntry e;
        e.url = url;
        e.title = title.isEmpty() ? url.toString() : title;
        e.visitTime = QDateTime::currentDateTime();
        e.visitCount = 1;
        m_entries.prepend(e);
    }

    // Trim
    while (m_entries.size() > MAX_ENTRIES)
        m_entries.removeLast();

    save();
    emit historyChanged();
}

QList<HistoryEntry> HistoryManager::recentHistory(int limit) const
{
    return m_entries.mid(0, qMin(limit, static_cast<int>(m_entries.size())));
}

QList<HistoryEntry> HistoryManager::searchHistory(const QString &query) const
{
    if (query.isEmpty()) return {};
    QString lower = query.toLower();
    QList<HistoryEntry> results;
    for (const auto &e : m_entries) {
        if (e.title.toLower().contains(lower) ||
            e.url.toString().toLower().contains(lower))
            results.append(e);
    }
    return results;
}

QList<HistoryEntry> HistoryManager::historyForDate(const QDate &date) const
{
    QList<HistoryEntry> results;
    for (const auto &e : m_entries) {
        if (e.visitTime.date() == date)
            results.append(e);
    }
    return results;
}

QMap<QDate, QList<HistoryEntry>> HistoryManager::groupedHistory() const
{
    QMap<QDate, QList<HistoryEntry>> grouped;
    for (const auto &e : m_entries) {
        grouped[e.visitTime.date()].append(e);
    }
    return grouped;
}

void HistoryManager::clearHistory()
{
    m_entries.clear();
    save();
    emit historyChanged();
}

void HistoryManager::clearRange(const QDate &from, const QDate &to)
{
    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
        [&](const HistoryEntry &e) {
            return e.visitTime.date() >= from && e.visitTime.date() <= to;
        }), m_entries.end());
    save();
    emit historyChanged();
}

void HistoryManager::removeEntry(const QUrl &url)
{
    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
        [&url](const HistoryEntry &e) { return e.url == url; }),
        m_entries.end());
    save();
    emit historyChanged();
}

int HistoryManager::count() const { return m_entries.size(); }

} // namespace Frint
