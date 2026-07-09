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

    // Load default preferences from JSON config
    void loadDefaults(const QString &configPath = QString());

    // Get/set individual settings
    QVariant getValue(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);

    // Convenience methods
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

    QString homePage() const;
    void setHomePage(const QString &url);

    // Sync settings to disk
    void sync();

signals:
    void settingChanged(const QString &key, const QVariant &value);

private:
    SettingsManager();
    QSettings m_settings;
    QJsonObject m_defaults;
};

} // namespace Frint

#endif // FRINT_SETTINGS_MANAGER_H
