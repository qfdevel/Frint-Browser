#ifndef FRINT_NOTE_TAKING_H
#define FRINT_NOTE_TAKING_H

#include <QObject>
#include <QString>
#include <QList>
#include <QDateTime>
#include <QColor>
#include <QJsonArray>

namespace Frint {

struct Note {
    QString id;
    QString title;
    QString content;
    QDateTime created;
    QDateTime modified;
    QColor color;
    bool pinned = false;
};

class NoteManager : public QObject {
    Q_OBJECT

public:
    static NoteManager &instance();

    QList<Note> notes() const;
    void addNote(const QString &title, const QString &content, const QColor &color = QColor());
    void updateNote(const QString &id, const QString &title, const QString &content);
    void removeNote(const QString &id);
    void togglePin(const QString &id);
    void setNoteColor(const QString &id, const QColor &color);
    Note findNote(const QString &id) const;
    int count() const;

signals:
    void notesChanged();

private:
    NoteManager();
    ~NoteManager() = default;
    NoteManager(const NoteManager &) = delete;
    NoteManager &operator=(const NoteManager &) = delete;

    void save();
    void load();
    QString storagePath() const;

    QList<Note> m_notes;
};

} // namespace Frint

#endif // FRINT_NOTE_TAKING_H
