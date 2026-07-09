#include "note_taking.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include <QDebug>

namespace Frint {

NoteManager &NoteManager::instance()
{
    static NoteManager s_instance;
    return s_instance;
}

NoteManager::NoteManager()
{
    load();
}

QString NoteManager::storagePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                  + "/Frint";
    QDir().mkpath(dir);
    return dir + "/notes.json";
}

void NoteManager::save()
{
    QJsonArray arr;
    for (const auto &n : m_notes) {
        QJsonObject obj;
        obj["id"] = n.id;
        obj["title"] = n.title;
        obj["content"] = n.content;
        obj["created"] = n.created.toString(Qt::ISODate);
        obj["modified"] = n.modified.toString(Qt::ISODate);
        obj["color"] = n.color.name();
        obj["pinned"] = n.pinned;
        arr.append(obj);
    }
    QJsonObject root;
    root["notes"] = arr;
    QFile file(storagePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void NoteManager::load()
{
    m_notes.clear();
    QFile file(storagePath());
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return;

    QJsonArray arr = doc.object()["notes"].toArray();
    for (const auto &val : arr) {
        QJsonObject obj = val.toObject();
        Note n;
        n.id = obj["id"].toString();
        n.title = obj["title"].toString();
        n.content = obj["content"].toString();
        n.created = QDateTime::fromString(obj["created"].toString(), Qt::ISODate);
        n.modified = QDateTime::fromString(obj["modified"].toString(), Qt::ISODate);
        n.color = QColor(obj["color"].toString());
        n.pinned = obj["pinned"].toBool();
        if (!n.id.isEmpty()) m_notes.append(n);
    }
}

QList<Note> NoteManager::notes() const
{
    QList<Note> sorted = m_notes;
    std::sort(sorted.begin(), sorted.end(), [](const Note &a, const Note &b) {
        if (a.pinned != b.pinned) return a.pinned;
        return a.modified > b.modified;
    });
    return sorted;
}

void NoteManager::addNote(const QString &title, const QString &content, const QColor &color)
{
    Note n;
    n.id = QUuid::createUuid().toString(QUuid::Id128);
    n.title = title.isEmpty() ? "Untitled Note" : title;
    n.content = content;
    n.created = QDateTime::currentDateTime();
    n.modified = n.created;
    n.color = color.isValid() ? color : QColor("#45475a");
    n.pinned = false;
    m_notes.prepend(n);
    save();
    emit notesChanged();
}

void NoteManager::updateNote(const QString &id, const QString &title, const QString &content)
{
    for (auto &n : m_notes) {
        if (n.id == id) {
            n.title = title.isEmpty() ? "Untitled Note" : title;
            n.content = content;
            n.modified = QDateTime::currentDateTime();
            break;
        }
    }
    save();
    emit notesChanged();
}

void NoteManager::removeNote(const QString &id)
{
    m_notes.erase(std::remove_if(m_notes.begin(), m_notes.end(),
        [&id](const Note &n) { return n.id == id; }), m_notes.end());
    save();
    emit notesChanged();
}

void NoteManager::togglePin(const QString &id)
{
    for (auto &n : m_notes) {
        if (n.id == id) {
            n.pinned = !n.pinned;
            break;
        }
    }
    save();
    emit notesChanged();
}

void NoteManager::setNoteColor(const QString &id, const QColor &color)
{
    for (auto &n : m_notes) {
        if (n.id == id) {
            n.color = color;
            break;
        }
    }
    save();
    emit notesChanged();
}

Note NoteManager::findNote(const QString &id) const
{
    for (const auto &n : m_notes) {
        if (n.id == id) return n;
    }
    return Note();
}

int NoteManager::count() const { return m_notes.size(); }

} // namespace Frint
