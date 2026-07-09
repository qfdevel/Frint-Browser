#ifndef FRINT_FINGERPRINTING_DEFENDER_H
#define FRINT_FINGERPRINTING_DEFENDER_H

#include <QString>
#include <QStringList>

namespace Frint {

class FingerprintingDefender {
public:
    static FingerprintingDefender &instance();

    void setEnabled(bool enabled);
    bool isEnabled() const;

    // Returns a fixed/spoofed font list (prevents font enumeration)
    QStringList spoofedFontFamilies() const;

    // Returns a consistent spoofed canvas fingerprint hash
    QString spoofedCanvasFingerprint() const;

    // Returns a spoofed user agent
    QString spoofedUserAgent() const;

    // Spoofed screen dimensions (rounded)
    int spoofedScreenWidth() const;
    int spoofedScreenHeight() const;

    // Spoofed timezone offset (round to nearest hour)
    int spoofedTimezoneOffset() const;

private:
    FingerprintingDefender();
    bool m_enabled = true;

    static QStringList s_fontFamilies;
};

} // namespace Frint

#endif // FRINT_FINGERPRINTING_DEFENDER_H
