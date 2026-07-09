#ifndef FRINT_SETTINGS_MANAGER_H
#define FRINT_SETTINGS_MANAGER_H

#include <QObject>
#include <QSettings>
#include <QJsonObject>
#include <QString>
#include <QVariant>
#include <QDir>

namespace Frint {

class SettingsManager : public QObject {
    Q_OBJECT

public:
    static SettingsManager &instance();

    void loadDefaults(const QString &configPath = QString());
    QVariant getValue(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);
    bool getBool(const QString &key, bool defaultValue = false) const;
    int getInt(const QString &key, int defaultValue = 0) const;
    QString getString(const QString &key, const QString &defaultValue = QString()) const;

    // Privacy
    bool isHttpsOnly() const;
    void setHttpsOnly(bool enabled);
    bool areThirdPartyCookiesBlocked() const;
    void setThirdPartyCookiesBlocked(bool blocked);
    bool isFingerprintingProtectionEnabled() const;
    void setFingerprintingProtection(bool enabled);
    bool isGpcEnabled() const;
    void setGpcEnabled(bool enabled);
    bool isClearOnExit() const;
    void setClearOnExit(bool enabled);
    bool isTrackingBlockerEnabled() const;
    void setTrackingBlockerEnabled(bool enabled);
    bool isDohEnabled() const;
    void setDohEnabled(bool enabled);
    bool isReferrerPolicyStrict() const;

    // Performance
    bool isHardwareAccelerationEnabled() const;
    void setHardwareAccelerationEnabled(bool enabled);
    bool isSSEAVXEnabled() const;
    void setSSEAVXEnabled(bool enabled);
    QString performanceMode() const;
    void setPerformanceMode(const QString &mode);

    // Advanced Privacy
    bool isCanvasFingerprintingBlocked() const;
    void setCanvasFingerprintingBlocked(bool blocked);
    bool isWebglFingerprintingBlocked() const;
    void setWebglFingerprintingBlocked(bool blocked);
    bool isAudioFingerprintingBlocked() const;
    void setAudioFingerprintingBlocked(bool blocked);
    bool isFontFingerprintingBlocked() const;
    void setFontFingerprintingBlocked(bool blocked);
    bool isSpoofHardwareConcurrency() const;
    void setSpoofHardwareConcurrency(bool enabled);
    bool isSpoofDeviceMemory() const;
    void setSpoofDeviceMemory(bool enabled);
    bool isForceUtcTimezone() const;
    void setForceUtcTimezone(bool enabled);
    bool isWebrtcBlocked() const;
    void setWebrtcBlocked(bool blocked);
    bool isEtagTrackingBlocked() const;
    void setEtagTrackingBlocked(bool blocked);
    bool isClientHintsBlocked() const;
    void setClientHintsBlocked(bool blocked);
    bool isAntiFingerprintEnabled() const;
    void setAntiFingerprintEnabled(bool enabled);

    // Downloads & Updates
    QString downloadLocation() const;
    void setDownloadLocation(const QString &path);
    bool isAlwaysAskDownloadLocation() const;
    void setAlwaysAskDownloadLocation(bool enabled);
    bool isAutoUpdateCheck() const;
    void setAutoUpdateCheck(bool enabled);

    // Browser
    QString homePage() const;
    void setHomePage(const QString &url);
    QString searchEngine() const;
    int contrastLevel() const;
    void setContrastLevel(int level);
    void setSearchEngine(const QString &url);
    QString theme() const;
    void setTheme(const QString &name);
    QString activeProfile() const;
    void setActiveProfile(const QString &name);
    QStringList profiles() const;
    void setProfiles(const QStringList &list);

    QString dohEndpoint() const;
    void setDohEndpoint(const QString &url);

    // Paths
    QString frintDataDir() const;
    QString profileDir() const;

    void sync();

signals:
    void settingChanged(const QString &key, const QVariant &value);
    void themeChanged(const QString &theme);
    void profileChanged(const QString &profile);

private:
    SettingsManager(QObject *parent = nullptr);
    SettingsManager(const SettingsManager &) = delete;
    SettingsManager &operator=(const SettingsManager &) = delete;

    QSettings m_settings;
    QJsonObject m_defaults;
    bool m_defaultsLoaded = false;
    mutable QString m_frintDir;
};

} // namespace Frint

#endif
