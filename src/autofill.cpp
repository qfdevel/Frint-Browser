#include "autofill.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

namespace Frint {

AutofillManager &AutofillManager::instance()
{
    static AutofillManager s_instance;
    return s_instance;
}

AutofillManager::AutofillManager()
{
    load();
}

QString AutofillManager::storagePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                  + "/Frint";
    QDir().mkpath(dir);
    return dir + "/autofill.json";
}

void AutofillManager::save()
{
    QJsonArray arr;
    for (const auto &e : m_entries) {
        QJsonObject obj;
        obj["type"] = e.type;
        obj["label"] = e.label;
        obj["fields"] = e.fields;
        arr.append(obj);
    }
    QJsonObject root;
    root["entries"] = arr;
    QFile file(storagePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void AutofillManager::load()
{
    m_entries.clear();
    QFile file(storagePath());
    if (!file.open(QIODevice::ReadOnly)) {
        // Create default sample entries
        addEntry("profile", "Default Profile", QJsonObject{
            {"name", "John Doe"},
            {"email", "john@example.com"},
            {"phone", "+1-555-1234"},
            {"address", "123 Main St"},
            {"city", "Anytown"},
            {"zip", "12345"},
            {"country", "US"}
        });
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return;

    QJsonArray arr = doc.object()["entries"].toArray();
    for (const auto &val : arr) {
        QJsonObject obj = val.toObject();
        AutofillEntry e;
        e.type = obj["type"].toString();
        e.label = obj["label"].toString();
        e.fields = obj["fields"].toObject();
        m_entries.append(e);
    }
}

void AutofillManager::addEntry(const QString &type, const QString &label, const QJsonObject &fields)
{
    // Prevent duplicate label+type
    for (auto &e : m_entries) {
        if (e.type == type && e.label == label) {
            e.fields = fields;
            save();
            emit autofillChanged();
            return;
        }
    }
    AutofillEntry e;
    e.type = type.isEmpty() ? "profile" : type;
    e.label = label;
    e.fields = fields;
    m_entries.append(e);
    save();
    emit autofillChanged();
}

void AutofillManager::removeEntry(const QString &type, const QString &label)
{
    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
        [&](const AutofillEntry &e) { return e.type == type && e.label == label; }),
        m_entries.end());
    save();
    emit autofillChanged();
}

QList<AutofillEntry> AutofillManager::entries(const QString &type) const
{
    if (type.isEmpty()) return m_entries;
    QList<AutofillEntry> result;
    for (const auto &e : m_entries) {
        if (e.type == type) result.append(e);
    }
    return result;
}

QJsonObject AutofillManager::getMatchingFields(const QString &type, const QString &hint) const
{
    Q_UNUSED(hint);
    // Return the first entry of the requested type
    for (const auto &e : m_entries) {
        if (e.type == type || type.isEmpty())
            return e.fields;
    }
    return {};
}

QString AutofillManager::generateAutofillJs(const QString &type) const
{
    QJsonObject fields = getMatchingFields(type, QString());
    if (fields.isEmpty()) return QString();

    QString js = "(function(){";
    js += "var fields={";
    QStringList parts;
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        parts.append(QString("'%1':'%2'")
            .arg(it.key()).arg(it.value().toString().replace("'", "\\'")));
    }
    js += parts.join(",");
    js += "};";

    js += R"(
var inputs=document.querySelectorAll('input,textarea,select');
for(var i=0;i<inputs.length;i++){
    var inp=inputs[i];
    var name=(inp.name||'').toLowerCase();
    var id=(inp.id||'').toLowerCase();
    var type=(inp.type||'').toLowerCase();
    for(var key in fields){
        var k=key.toLowerCase();
        if(name.includes(k)||id.includes(k)){
            inp.value=fields[key];
            inp.dispatchEvent(new Event('input',{bubbles:true}));
            inp.dispatchEvent(new Event('change',{bubbles:true}));
            break;
        }
    }
}
)";
    js += "})();";
    return js;
}

} // namespace Frint
