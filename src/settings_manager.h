#ifndef FRINT_SETTINGS_MANAGER_H
#define FRINT_SETTINGS_MANAGER_H

#include <QObject>
#include <QSettings>
#include <QJsonObject>
#include <QString>
#include <QVariant>

namespace Frint {

class SettingsManager : public QObject {
    Q_OBJECT

public:
    static SettingsManager &instance();

    // Load default preferences from JSON file
    void loadDefaults(const QString &configPath = QString());

    // Get/set settings with types
    QVariant getValue(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);

    bool getBool(const QString &key, bool defaultValue = false) const;
    int getInt(const QString &key, int defaultValue = 0) const;
    QString getString(const QString &key,
                      const QString &defaultValue = QString()) const;

    // Privacy convenience accessors
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

    QString homePage() const;
    void setHomePage(const QString &url);

    QString searchEngine() const;

    QString dohEndpoint() const;
    void setDohEndpoint(const QString &url);

    // Persist to disk
    void sync();

signals:
    void settingChanged(const QString &key, const QVariant &value);

private:
    SettingsManager(QObject *parent = nullptr);
    SettingsManager(const SettingsManager &) = delete;
    SettingsManager &operator=(const SettingsManager &) = delete;

    QSettings m_settings;
    QJsonObject m_defaults;
    bool m_defaultsLoaded = false;
};

} // namespace Frint

#endif // FRINT_SETTINGS_MANAGER_H
