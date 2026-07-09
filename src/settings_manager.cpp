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

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
    , m_settings(QSettings::IniFormat, QSettings::UserScope,
                 "FrintBrowser", "FrintBrowser")
{
}

void SettingsManager::loadDefaults(const QString &configPath)
{
    QString path = configPath;

    if (path.isEmpty()) {
        QString appDir = QCoreApplication::applicationDirPath();
        QStringList searchPaths = {
            appDir + "/../configs/default_prefs.json",
            appDir + "/configs/default_prefs.json",
            QDir::currentPath() + "/configs/default_prefs.json",
            QStandardPaths::locate(QStandardPaths::AppConfigLocation,
                                   "default_prefs.json"),
        };

        for (const QString &sp : searchPaths) {
            if (QFile::exists(sp)) {
                path = sp;
                break;
            }
        }
    }

    if (path.isEmpty() || !QFile::exists(path)) {
        qWarning() << "[Frint Settings] default_prefs.json not found,"
                    << "using hardcoded defaults";
        // Set some sensible defaults directly
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[Frint Settings] Cannot open" << path;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "[Frint Settings] JSON parse error:" << error.errorString();
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "[Frint Settings] Expected JSON object";
        return;
    }

    m_defaults = doc.object();

    // Apply defaults for unset keys
    for (auto it = m_defaults.constBegin(); it != m_defaults.constEnd(); ++it) {
        if (!m_settings.contains(it.key())) {
            m_settings.setValue(it.key(), it.value().toVariant());
        }
    }

    m_defaultsLoaded = true;
    qDebug() << "[Frint Settings] Loaded" << m_defaults.size()
             << "default settings from" << path;
}

// ── Generic get/set ──────────────────────────────────────────────────────

QVariant SettingsManager::getValue(const QString &key,
                                   const QVariant &defaultValue) const
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

bool SettingsManager::getBool(const QString &key, bool defaultValue) const
{
    return getValue(key, defaultValue).toBool();
}

int SettingsManager::getInt(const QString &key, int defaultValue) const
{
    return getValue(key, defaultValue).toInt();
}

QString SettingsManager::getString(const QString &key,
                                   const QString &defaultValue) const
{
    return getValue(key, defaultValue).toString();
}

// ── Privacy convenience methods ──────────────────────────────────────────

bool SettingsManager::isHttpsOnly() const
{
    return getBool("https_only", true);
}

void SettingsManager::setHttpsOnly(bool enabled)
{
    setValue("https_only", enabled);
}

bool SettingsManager::areThirdPartyCookiesBlocked() const
{
    return getBool("third_party_cookies_blocked", true);
}

void SettingsManager::setThirdPartyCookiesBlocked(bool blocked)
{
    setValue("third_party_cookies_blocked", blocked);
}

bool SettingsManager::isFingerprintingProtectionEnabled() const
{
    return getBool("fingerprinting_protection", true);
}

void SettingsManager::setFingerprintingProtection(bool enabled)
{
    setValue("fingerprinting_protection", enabled);
}

bool SettingsManager::isGpcEnabled() const
{
    return getBool("gpc_enabled", true);
}

void SettingsManager::setGpcEnabled(bool enabled)
{
    setValue("gpc_enabled", enabled);
}

bool SettingsManager::isClearOnExit() const
{
    return getBool("clear_on_exit", true);
}

void SettingsManager::setClearOnExit(bool enabled)
{
    setValue("clear_on_exit", enabled);
}

bool SettingsManager::isTrackingBlockerEnabled() const
{
    return getBool("tracking_blocker", true);
}

void SettingsManager::setTrackingBlockerEnabled(bool enabled)
{
    setValue("tracking_blocker", enabled);
}

bool SettingsManager::isDohEnabled() const
{
    return getBool("doh_enabled", true);
}

void SettingsManager::setDohEnabled(bool enabled)
{
    setValue("doh_enabled", enabled);
}

bool SettingsManager::isReferrerPolicyStrict() const
{
    return getString("referrer_policy", "strict-origin-when-cross-origin")
           == "strict-origin-when-cross-origin";
}

QString SettingsManager::homePage() const
{
    return getString("home_page", "about:blank");
}

void SettingsManager::setHomePage(const QString &url)
{
    setValue("home_page", url);
}

QString SettingsManager::searchEngine() const
{
    return getString("search_engine",
                     "https://duckduckgo.com/?q=");
}

QString SettingsManager::dohEndpoint() const
{
    return getString("doh_endpoint",
                     "https://cloudflare-dns.com/dns-query");
}

void SettingsManager::setDohEndpoint(const QString &url)
{
    setValue("doh_endpoint", url);
}

void SettingsManager::sync()
{
    m_settings.sync();
}

} // namespace Frint
