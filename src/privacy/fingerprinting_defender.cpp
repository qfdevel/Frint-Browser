#include "privacy/fingerprinting_defender.h"
#include <QRandomGenerator>
#include <QDateTime>

namespace Frint {

QStringList FingerprintingDefender::s_fontFamilies = {
    "Arial", "Arial Black", "Comic Sans MS", "Courier New",
    "Georgia", "Impact", "Lucida Console", "Lucida Sans Unicode",
    "Palatino Linotype", "Tahoma", "Times New Roman",
    "Trebuchet MS", "Verdana", "Segoe UI", "Roboto",
    "Open Sans", "Helvetica", "sans-serif", "serif", "monospace"
};

FingerprintingDefender &FingerprintingDefender::instance()
{
    static FingerprintingDefender s_instance;
    return s_instance;
}

FingerprintingDefender::FingerprintingDefender() = default;

void FingerprintingDefender::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool FingerprintingDefender::isEnabled() const
{
    return m_enabled;
}

QStringList FingerprintingDefender::spoofedFontFamilies() const
{
    if (!m_enabled) {
        return QStringList(); // Return empty - real system fonts would be used
    }
    return s_fontFamilies;
}

QString FingerprintingDefender::spoofedCanvasFingerprint() const
{
    if (!m_enabled) {
        return QString();
    }
    // Return a fixed, deterministic canvas fingerprint
    // This defeats canvas fingerprinting by returning the same hash every time
    return "frint-fixed-canvas-hash-00000000000000000000000000000000";
}

QString FingerprintingDefender::spoofedUserAgent() const
{
    if (!m_enabled) {
        return QString();
    }
    // Return a common user agent to blend in
    return "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
           "(KHTML, like Gecko) FrintBrowser/0.1.0 Chrome/120.0.0.0 Safari/537.36";
}

int FingerprintingDefender::spoofedScreenWidth() const
{
    if (!m_enabled) {
        return 0;
    }
    // Round to nearest common resolution to reduce uniqueness
    return 1920;
}

int FingerprintingDefender::spoofedScreenHeight() const
{
    if (!m_enabled) {
        return 0;
    }
    return 1080;
}

int FingerprintingDefender::spoofedTimezoneOffset() const
{
    if (!m_enabled) {
        return QDateTime::currentDateTime().offsetFromUtc();
    }
    // Round timezone offset to nearest hour to reduce precision
    int offset = QDateTime::currentDateTime().offsetFromUtc();
    int rounded = (offset / 3600) * 3600;
    return rounded;
}

} // namespace Frint
