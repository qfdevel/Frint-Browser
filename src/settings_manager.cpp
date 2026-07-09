#include "settings_manager.h"
#include <QFile>
#include <QJsonDocument>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDebug>
#include <QCryptographicHash>

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

QString SettingsManager::frintDataDir() const
{
    if (m_frintDir.isEmpty()) {
        m_frintDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                     + "/Frint";
        QDir().mkpath(m_frintDir);
        QDir().mkpath(m_frintDir + "/profiles");
        QDir().mkpath(m_frintDir + "/themes");
        QDir().mkpath(m_frintDir + "/extensions");
        qDebug() << "[Frint] Data directory:" << m_frintDir;
    }
    return m_frintDir;
}

QString SettingsManager::profileDir() const
{
    QString profile = activeProfile();
    QString dir = frintDataDir() + "/profiles/" + profile;
    QDir().mkpath(dir);
    return dir;
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
        };
        for (const QString &sp : searchPaths) {
            if (QFile::exists(sp)) { path = sp; break; }
        }
    }

    if (path.isEmpty() || !QFile::exists(path)) {
        qWarning() << "[Frint] Using hardcoded defaults";
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { qWarning() << "[Frint] Cannot open" << path; return; }
    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) { qWarning() << "[Frint] JSON parse error:" << error.errorString(); return; }
    if (!doc.isObject()) { qWarning() << "[Frint] Expected JSON object"; return; }

    m_defaults = doc.object();
    for (auto it = m_defaults.constBegin(); it != m_defaults.constEnd(); ++it) {
        if (!m_settings.contains(it.key()))
            m_settings.setValue(it.key(), it.value().toVariant());
    }
    m_defaultsLoaded = true;
    qDebug() << "[Frint] Loaded" << m_defaults.size() << "defaults from" << path;
}

QVariant SettingsManager::getValue(const QString &key, const QVariant &defaultValue) const
{
    if (m_settings.contains(key)) return m_settings.value(key);
    if (m_defaults.contains(key)) return m_defaults[key].toVariant();
    return defaultValue;
}

void SettingsManager::setValue(const QString &key, const QVariant &value) { m_settings.setValue(key, value); emit settingChanged(key, value); }
bool SettingsManager::getBool(const QString &key, bool d) const { return getValue(key, d).toBool(); }
int SettingsManager::getInt(const QString &key, int d) const { return getValue(key, d).toInt(); }
QString SettingsManager::getString(const QString &key, const QString &d) const { return getValue(key, d).toString(); }

bool SettingsManager::isHttpsOnly() const { return getBool("https_only", true); }
void SettingsManager::setHttpsOnly(bool e) { setValue("https_only", e); }
bool SettingsManager::areThirdPartyCookiesBlocked() const { return getBool("third_party_cookies_blocked", true); }
void SettingsManager::setThirdPartyCookiesBlocked(bool b) { setValue("third_party_cookies_blocked", b); }
bool SettingsManager::isFingerprintingProtectionEnabled() const { return getBool("fingerprinting_protection", true); }
void SettingsManager::setFingerprintingProtection(bool e) { setValue("fingerprinting_protection", e); }
bool SettingsManager::isGpcEnabled() const { return getBool("gpc_enabled", true); }
void SettingsManager::setGpcEnabled(bool e) { setValue("gpc_enabled", e); }
bool SettingsManager::isClearOnExit() const { return getBool("clear_on_exit", true); }
void SettingsManager::setClearOnExit(bool e) { setValue("clear_on_exit", e); }
bool SettingsManager::isTrackingBlockerEnabled() const { return getBool("tracking_blocker", true); }
void SettingsManager::setTrackingBlockerEnabled(bool e) { setValue("tracking_blocker", e); }
bool SettingsManager::isDohEnabled() const { return getBool("doh_enabled", true); }
void SettingsManager::setDohEnabled(bool e) { setValue("doh_enabled", e); }
bool SettingsManager::isReferrerPolicyStrict() const { return true; }
bool SettingsManager::isCanvasFingerprintingBlocked() const { return getBool("canvas_fingerprinting_blocked", true); }
void SettingsManager::setCanvasFingerprintingBlocked(bool b) { setValue("canvas_fingerprinting_blocked", b); }
bool SettingsManager::isWebglFingerprintingBlocked() const { return getBool("webgl_fingerprinting_blocked", true); }
void SettingsManager::setWebglFingerprintingBlocked(bool b) { setValue("webgl_fingerprinting_blocked", b); }
bool SettingsManager::isAudioFingerprintingBlocked() const { return getBool("audio_fingerprinting_blocked", true); }
void SettingsManager::setAudioFingerprintingBlocked(bool b) { setValue("audio_fingerprinting_blocked", b); }
bool SettingsManager::isFontFingerprintingBlocked() const { return getBool("font_fingerprinting_blocked", true); }
void SettingsManager::setFontFingerprintingBlocked(bool b) { setValue("font_fingerprinting_blocked", b); }
bool SettingsManager::isSpoofHardwareConcurrency() const { return getBool("spoof_hardware_concurrency", true); }
void SettingsManager::setSpoofHardwareConcurrency(bool e) { setValue("spoof_hardware_concurrency", e); }
bool SettingsManager::isSpoofDeviceMemory() const { return getBool("spoof_device_memory", true); }
void SettingsManager::setSpoofDeviceMemory(bool e) { setValue("spoof_device_memory", e); }
bool SettingsManager::isForceUtcTimezone() const { return getBool("force_utc_timezone", true); }
void SettingsManager::setForceUtcTimezone(bool e) { setValue("force_utc_timezone", e); }
bool SettingsManager::isWebrtcBlocked() const { return getBool("webrtc_blocked", true); }
void SettingsManager::setWebrtcBlocked(bool b) { setValue("webrtc_blocked", b); }
bool SettingsManager::isEtagTrackingBlocked() const { return getBool("etag_tracking_blocked", true); }
void SettingsManager::setEtagTrackingBlocked(bool b) { setValue("etag_tracking_blocked", b); }
bool SettingsManager::isClientHintsBlocked() const { return getBool("client_hints_blocked", true); }
void SettingsManager::setClientHintsBlocked(bool b) { setValue("client_hints_blocked", b); }

QString SettingsManager::downloadLocation() const { return getString("download_location", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)); }
void SettingsManager::setDownloadLocation(const QString &p) { setValue("download_location", p); }
bool SettingsManager::isAlwaysAskDownloadLocation() const { return getBool("always_ask_download_location", false); }
void SettingsManager::setAlwaysAskDownloadLocation(bool e) { setValue("always_ask_download_location", e); }
bool SettingsManager::isAutoUpdateCheck() const { return getBool("auto_update_check", true); }
void SettingsManager::setAutoUpdateCheck(bool e) { setValue("auto_update_check", e); }

QString SettingsManager::homePage() const { return getString("home_page", "https://duckduckgo.com"); }
void SettingsManager::setHomePage(const QString &u) { setValue("home_page", u); }
int SettingsManager::contrastLevel() const
{
    return getInt("contrast_level", 50);
}

void SettingsManager::setContrastLevel(int level)
{
    setValue("contrast_level", qBound(0, level, 100));
}

QString SettingsManager::searchEngine() const { return getString("search_engine", "https://duckduckgo.com/?q="); }
void SettingsManager::setSearchEngine(const QString &u) { setValue("search_engine", u); }
QString SettingsManager::theme() const { return getString("theme", "catppuccin-mocha"); }
void SettingsManager::setTheme(const QString &n) { setValue("theme", n); emit themeChanged(n); }
QString SettingsManager::activeProfile() const { return getString("active_profile", "Default"); }
void SettingsManager::setActiveProfile(const QString &n) { setValue("active_profile", n); emit profileChanged(n); }
QStringList SettingsManager::profiles() const { return getString("profiles_list", "Default").split(',', Qt::SkipEmptyParts); }
void SettingsManager::setProfiles(const QStringList &l) { setValue("profiles_list", l.join(',')); }

QString SettingsManager::dohEndpoint() const { return getString("doh_endpoint", "https://cloudflare-dns.com/dns-query"); }
void SettingsManager::setDohEndpoint(const QString &u) { setValue("doh_endpoint", u); }
bool SettingsManager::isHardwareAccelerationEnabled() const { return getBool("hardware_acceleration", true); }
void SettingsManager::setHardwareAccelerationEnabled(bool e) { setValue("hardware_acceleration", e); }
bool SettingsManager::isSSEAVXEnabled() const { return getBool("sse_avx_optimizations", true); }
void SettingsManager::setSSEAVXEnabled(bool e) { setValue("sse_avx_optimizations", e); }
QString SettingsManager::performanceMode() const { return getString("performance_mode", "maximum"); }
void SettingsManager::setPerformanceMode(const QString &m) { setValue("performance_mode", m); }

bool SettingsManager::isAntiFingerprintEnabled() const { return getBool("anti_fingerprint_enabled", true); }
void SettingsManager::setAntiFingerprintEnabled(bool e) { setValue("anti_fingerprint_enabled", e); }

void SettingsManager::sync() { m_settings.sync(); }

} // namespace Frint
