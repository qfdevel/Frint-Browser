#include "privacy/fingerprinting_defender.h"
#include <QDebug>
#include <QDateTime>

namespace Frint {

FingerprintingDefender &FingerprintingDefender::instance()
{
    static FingerprintingDefender s_instance;
    return s_instance;
}

FingerprintingDefender::FingerprintingDefender()
    : m_spoofedCanvasHash("frint-canvas-fingerprint-protected")
    , m_spoofedUA("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
                  "(KHTML, like Gecko) FrintBrowser/0.1.0 Chrome/120.0.0.0 Safari/537.36")
{
    m_spoofedFonts = {
        "Arial", "Arial Black", "Comic Sans MS", "Courier New",
        "Georgia", "Impact", "Lucida Console", "Lucida Sans Unicode",
        "Palatino Linotype", "Tahoma", "Times New Roman",
        "Trebuchet MS", "Verdana", "Segoe UI", "Roboto",
        "Open Sans", "Helvetica", "sans-serif", "serif", "monospace"
    };
}

void FingerprintingDefender::setEnabled(bool enabled)
{
    m_enabled = enabled;
    qDebug() << "[Frint FingerprintingDefender]"
             << (enabled ? "Enabled" : "Disabled");
}

bool FingerprintingDefender::isEnabled() const
{
    return m_enabled;
}

// ── Font spoofing ────────────────────────────────────────────────────────

QStringList FingerprintingDefender::spoofedFontFamilies() const
{
    if (!m_enabled) return {};
    return m_spoofedFonts;
}

void FingerprintingDefender::setSpoofedFonts(const QStringList &fonts)
{
    m_spoofedFonts = fonts;
}

// ── Canvas fingerprinting ────────────────────────────────────────────────

QString FingerprintingDefender::spoofedCanvasFingerprint() const
{
    if (!m_enabled) return {};
    return m_spoofedCanvasHash;
}

void FingerprintingDefender::setCanvasNoiseEnabled(bool enabled)
{
    Q_UNUSED(enabled);
    // Canvas hash stays fixed regardless
}

// ── Screen resolution ────────────────────────────────────────────────────

int FingerprintingDefender::spoofedScreenWidth() const
{
    if (!m_enabled) return 0;
    return m_spoofedWidth;
}

int FingerprintingDefender::spoofedScreenHeight() const
{
    if (!m_enabled) return 0;
    return m_spoofedHeight;
}

void FingerprintingDefender::setSpoofedResolution(int width, int height)
{
    m_spoofedWidth = width;
    m_spoofedHeight = height;
}

// ── Timezone ─────────────────────────────────────────────────────────────

int FingerprintingDefender::spoofedTimezoneOffset() const
{
    if (!m_enabled) {
        return QDateTime::currentDateTime().offsetFromUtc();
    }
    return m_spoofedTimezoneOffset;
}

void FingerprintingDefender::setSpoofedTimezone(int offsetMinutes)
{
    m_spoofedTimezoneOffset = offsetMinutes;
}

// ── Language ─────────────────────────────────────────────────────────────

QString FingerprintingDefender::spoofedLanguage() const
{
    if (!m_enabled) return {};
    return m_spoofedLanguage;
}

void FingerprintingDefender::setSpoofedLanguage(const QString &lang)
{
    m_spoofedLanguage = lang;
}

// ── Hardware concurrency ─────────────────────────────────────────────────

int FingerprintingDefender::spoofedHardwareConcurrency() const
{
    if (!m_enabled) return 0;
    return m_spoofedConcurrency;
}

void FingerprintingDefender::setSpoofedConcurrency(int count)
{
    m_spoofedConcurrency = qBound(2, count, 16);
}

// ── Device memory ────────────────────────────────────────────────────────

double FingerprintingDefender::spoofedDeviceMemory() const
{
    if (!m_enabled) return 0;
    return m_spoofedMemory;
}

void FingerprintingDefender::setSpoofedMemory(double gb)
{
    m_spoofedMemory = qBound(0.25, gb, 512.0);
}

// ── User-Agent ───────────────────────────────────────────────────────────

QString FingerprintingDefender::spoofedUserAgent() const
{
    if (!m_enabled) return {};
    return m_spoofedUA;
}

void FingerprintingDefender::setSpoofedUserAgent(const QString &ua)
{
    m_spoofedUA = ua;
}

// ── Apply settings from JSON ─────────────────────────────────────────────

void FingerprintingDefender::applySettings(const QJsonObject &settings)
{
    if (settings.contains("font_spoofing_enabled")) {
        setEnabled(settings["font_spoofing_enabled"].toBool());
    }
    if (settings.contains("canvas_fingerprinting_defense_enabled")) {
        setCanvasNoiseEnabled(
            settings["canvas_fingerprinting_defense_enabled"].toBool());
    }
    if (settings.contains("screen_resolution_spoofing_enabled")
        && settings["screen_resolution_spoofing_enabled"].toBool()) {
        setSpoofedResolution(1920, 1080);
    }
    if (settings.contains("timezone_spoofing_enabled")
        && settings["timezone_spoofing_enabled"].toBool()) {
        setSpoofedTimezone(0); // UTC
    }
    if (settings.contains("user_agent_spoofing_enabled")
        && settings["user_agent_spoofing_enabled"].toBool()) {
        // Use default spoofed UA
    }
}

} // namespace Frint
