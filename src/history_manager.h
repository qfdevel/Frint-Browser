#ifndef FRINT_HISTORY_MANAGER_H
#define FRINT_HISTORY_MANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QUrl>
#include <QDate>
#include <QJsonArray>

namespace Frint {

struct HistoryEntry {
    QUrl url;
    QString title;
    QDateTime visitTime;
    int visitCount = 1;
};

class HistoryManager : public QObject {
    Q_OBJECT

public:
    static HistoryManager &instance();

    void addVisit(const QUrl &url, const QString &title);
    QList<HistoryEntry> recentHistory(int limit = 100) const;
    QList<HistoryEntry> searchHistory(const QString &query) const;
    QList<HistoryEntry> historyForDate(const QDate &date) const;
    QMap<QDate, QList<HistoryEntry>> groupedHistory() const;
    void clearHistory();
    void clearRange(const QDate &from, const QDate &to);
    void removeEntry(const QUrl &url);

    int count() const;

signals:
    void historyChanged();

private:
    HistoryManager();
    ~HistoryManager() = default;
    HistoryManager(const HistoryManager &) = delete;
    HistoryManager &operator=(const HistoryManager &) = delete;

    void save();
    void load();
    QString storagePath() const;

    QList<HistoryEntry> m_entries;
    static constexpr int MAX_ENTRIES = 5000;
};

} // namespace Frint

#endif // FRINT_HISTORY_MANAGER_H
