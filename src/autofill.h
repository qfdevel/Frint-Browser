#ifndef FRINT_AUTOFILL_H
#define FRINT_AUTOFILL_H

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>

namespace Frint {

struct AutofillEntry {
    QString type;   // "address", "email", "phone", "name", "creditcard"
    QString label;
    QJsonObject fields; // field_name -> value
};

class AutofillManager : public QObject {
    Q_OBJECT

public:
    static AutofillManager &instance();

    void addEntry(const QString &type, const QString &label, const QJsonObject &fields);
    void removeEntry(const QString &type, const QString &label);
    QList<AutofillEntry> entries(const QString &type = QString()) const;
    QJsonObject getMatchingFields(const QString &type, const QString &hint) const;

    QString generateAutofillJs(const QString &type = QString()) const;

signals:
    void autofillChanged();

private:
    AutofillManager();
    ~AutofillManager() = default;
    AutofillManager(const AutofillManager &) = delete;
    AutofillManager &operator=(const AutofillManager &) = delete;

    void save();
    void load();
    QString storagePath() const;

    QList<AutofillEntry> m_entries;
};

} // namespace Frint

#endif // FRINT_AUTOFILL_H
