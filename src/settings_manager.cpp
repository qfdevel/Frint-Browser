#include "settings_manager.h"
#include <QFile>
#include <QJsonDocument>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDebug>

namespace Frint {

SettingsManager &SettingsManager::instance()
{
    static SettingsManager s_instance;
    return s_instance;
}

SettingsManager::SettingsManager()
    : QObject(nullptr)
    , m_settings(QSettings::IniFormat, QSettings::UserScope,
                 "FrintBrowser", "FrintBrowser")
{
}

void SettingsManager::loadDefaults(const QString &configPath)
{
    QString path = configPath;

    if (path.isEmpty()) {
        // Try to find configs relative to the executable
        QString appDir = QCoreApplication::applicationDirPath();
        QStringList searchPaths = {
            appDir + "/../configs/default_prefs.json",
            appDir + "/configs/default_prefs.json",
            QDir::currentPath() + "/configs/default_prefs.json",
            QStandardPaths::locate(QStandardPaths::AppConfigLocation,
                                   "default_prefs.json")
        };

        for (const QString &sp : searchPaths) {
            if (QFile::exists(sp)) {
                path = sp;
                break;
            }
        }
    }

    if (path.isEmpty()) {
        qWarning() << "[Frint] Could not find default_prefs.json, using built-in defaults";
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[Frint] Failed to open" << path;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "[Frint] JSON parse error in" << path << ":" << error.errorString();
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "[Frint] Expected JSON object in" << path;
        return;
    }

    m_defaults = doc.object();
    qDebug() << "[Frint] Loaded" << m_defaults.size() << "default settings from" << path;

    // Apply defaults for any keys not already set
    for (auto it = m_defaults.constBegin(); it != m_defaults.constEnd(); ++it) {
        if (!m_settings.contains(it.key())) {
            m_settings.setValue(it.key(), it.value().toVariant());
        }
    }
}

QVariant SettingsManager::getValue(const QString &key, const QVariant &defaultValue) const
{
    if (m_settings.contains(key)) {
        return m_settings.value(key);
    }
    if (m_defaults.contains(key)) {
        return m_defaults[key].toVariant();
    }
    return defaultValue;
}

void SettingsManager::setValue(const QString &key, const QVariant &value)
{
    m_settings.setValue(key, value);
    emit settingChanged(key, value);
}

bool SettingsManager::isHttpsOnly() const
{
    return getValue("https_only", true).toBool();
}

void SettingsManager::setHttpsOnly(bool enabled)
{
    setValue("https_only", enabled);
}

bool SettingsManager::areThirdPartyCookiesBlocked() const
{
    return getValue("third_party_cookies_blocked", true).toBool();
}

void SettingsManager::setThirdPartyCookiesBlocked(bool blocked)
{
    setValue("third_party_cookies_blocked", blocked);
}

bool SettingsManager::isFingerprintingProtectionEnabled() const
{
    return getValue("fingerprinting_protection", true).toBool();
}

void SettingsManager::setFingerprintingProtection(bool enabled)
{
    setValue("fingerprinting_protection", enabled);
}

bool SettingsManager::isGpcEnabled() const
{
    return getValue("gpc_enabled", true).toBool();
}

void SettingsManager::setGpcEnabled(bool enabled)
{
    setValue("gpc_enabled", enabled);
}

bool SettingsManager::isClearOnExit() const
{
    return getValue("clear_on_exit", false).toBool();
}

void SettingsManager::setClearOnExit(bool enabled)
{
    setValue("clear_on_exit", enabled);
}

bool SettingsManager::isTrackingBlockerEnabled() const
{
    return getValue("tracking_blocker", true).toBool();
}

void SettingsManager::setTrackingBlockerEnabled(bool enabled)
{
    setValue("tracking_blocker", enabled);
}

bool SettingsManager::isDohEnabled() const
{
    return getValue("doh_enabled", true).toBool();
}

void SettingsManager::setDohEnabled(bool enabled)
{
    setValue("doh_enabled", enabled);
}

QString SettingsManager::homePage() const
{
    return getValue("home_page", "about:blank").toString();
}

void SettingsManager::setHomePage(const QString &url)
{
    setValue("home_page", url);
}

void SettingsManager::sync()
{
    m_settings.sync();
}

} // namespace Frint
