#ifndef FRINT_FINGERPRINTING_DEFENDER_H
#define FRINT_FINGERPRINTING_DEFENDER_H

#include <QString>
#include <QStringList>
#include <QJsonObject>

namespace Frint {

class FingerprintingDefender {
public:
    static FingerprintingDefender &instance();

    void setEnabled(bool enabled);
    bool isEnabled() const;

    // Font fingerprinting
    QStringList spoofedFontFamilies() const;
    void setSpoofedFonts(const QStringList &fonts);

    // Canvas fingerprinting
    QString spoofedCanvasFingerprint() const;
    void setCanvasNoiseEnabled(bool enabled);

    // Screen resolution
    int spoofedScreenWidth() const;
    int spoofedScreenHeight() const;
    void setSpoofedResolution(int width, int height);

    // Timezone
    int spoofedTimezoneOffset() const;
    void setSpoofedTimezone(int offsetMinutes);

    // Language
    QString spoofedLanguage() const;
    void setSpoofedLanguage(const QString &lang);

    // Hardware concurrency
    int spoofedHardwareConcurrency() const;
    void setSpoofedConcurrency(int count);

    // Device memory
    double spoofedDeviceMemory() const;
    void setSpoofedMemory(double gb);

    // User-Agent
    QString spoofedUserAgent() const;
    void setSpoofedUserAgent(const QString &ua);

    // Apply all spoofing settings from JSON config
    void applySettings(const QJsonObject &settings);

private:
    FingerprintingDefender();

    bool m_enabled = true;
    QStringList m_spoofedFonts;
    QString m_spoofedCanvasHash;
    int m_spoofedWidth = 1920;
    int m_spoofedHeight = 1080;
    int m_spoofedTimezoneOffset = 0;
    QString m_spoofedLanguage = "en-US";
    int m_spoofedConcurrency = 4;
    double m_spoofedMemory = 8.0;
    QString m_spoofedUA;
};

} // namespace Frint

#endif // FRINT_FINGERPRINTING_DEFENDER_H
