#ifndef FRINT_QUICK_DIAL_H
#define FRINT_QUICK_DIAL_H

#include <QObject>
#include <QString>
#include <QList>
#include <QUrl>
#include <QJsonArray>

namespace Frint {

struct QuickDialEntry {
    QString title;
    QUrl url;
    QString iconPath;
};

class QuickDial : public QObject {
    Q_OBJECT

public:
    static QuickDial &instance();

    QString generateHtml() const;
    QList<QuickDialEntry> entries() const;
    void addEntry(const QString &title, const QUrl &url);
    void removeEntry(const QUrl &url);
    void reorder(int from, int to);
    void setEntries(const QList<QuickDialEntry> &entries);
    void setEntryIcon(int index, const QString &iconPath);

signals:
    void entriesChanged();

private:
    QuickDial();
    ~QuickDial() = default;
    QuickDial(const QuickDial &) = delete;
    QuickDial &operator=(const QuickDial &) = delete;

    void save();
    void load();
    QString storagePath() const;

    QList<QuickDialEntry> m_entries;
};

} // namespace Frint

#endif // FRINT_QUICK_DIAL_H
