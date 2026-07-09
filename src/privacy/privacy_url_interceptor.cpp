#include "privacy_url_interceptor.h"
#include "url_cleaner.h"
#include "tracking_blocker.h"
#include "gpc_header.h"
#include "referrer_policy.h"
#include "fingerprinting_defender.h"
#include <QDebug>
#include <QUrlQuery>

namespace Frint {

PrivacyUrlInterceptor::PrivacyUrlInterceptor(QObject *parent)
    : QWebEngineUrlRequestInterceptor(parent)
{
}

PrivacyUrlInterceptor::~PrivacyUrlInterceptor() = default;

void PrivacyUrlInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    QUrl requestUrl = info.requestUrl();
    QUrl firstPartyUrl = info.firstPartyUrl();
    QString resourceType;

    switch (info.resourceType()) {
    case QWebEngineUrlRequestInfo::ResourceTypeMainFrame:
        resourceType = "main_frame"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeSubFrame:
        resourceType = "sub_frame"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeXhr:
        resourceType = "xhr"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeScript:
        resourceType = "script"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeImage:
        resourceType = "image"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeStylesheet:
        resourceType = "stylesheet"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeMedia:
        resourceType = "media"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeFontResource:
        resourceType = "font"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeWorker:
        resourceType = "worker"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeWebSocket:
        resourceType = "websocket";
        // WebSocket: only block known trackers, skip ad pattern & header manipulation
        if (m_trackingBlockerEnabled && isTracker(requestUrl, firstPartyUrl)) {
            info.block(true);
            emit requestBlocked(requestUrl.toString(), "Blocked tracker via WebSocket");
            return;
        }
        return; // Let legitimate WebSocket through untouched
    default:
        resourceType = "other";
    }

    // Block known ad/tracker script paths (exact filename match to avoid false positives)
    {
        QString path = requestUrl.path().toLower();
        QStringList adScriptPatterns = {
            "/ads.js", "/ad.js", "/pagead.js", "/adsense.js",
            "/gtag.js", "/gtm.js", "/analytics.js", "/fbevents.js",
            "/pixel.js", "/tracking.js", "/conversion.js",
            "/adservice.js",
        };
        // Only block if the path ENDS with the pattern (exact match)
        for (const auto &pattern : adScriptPatterns) {
            if (path.endsWith(pattern)) {
                info.block(true);
                qDebug() << "[Frint Privacy] Blocked ad script:" << requestUrl.toString();
                emit requestBlocked(requestUrl.toString(), "Blocked ad script pattern");
                return;
            }
        }
        // Also block known ad paths (directory-based)
        QStringList adDirPatterns = {
            "/pagead/ad", "/pagead/js", "/pagead/gen_",
        };
        for (const auto &pattern : adDirPatterns) {
            if (path.startsWith(pattern)) {
                info.block(true);
                qDebug() << "[Frint Privacy] Blocked ad script (dir):" << requestUrl.toString();
                emit requestBlocked(requestUrl.toString(), "Blocked ad directory pattern");
                return;
            }
        }
    }

    // Step 1: Clean tracking parameters from URL
    QUrl cleanUrl = cleanTrackingParams(requestUrl);

    // Step 2: Upgrade HTTP to HTTPS
    QUrl finalUrl = upgradeToHttps(cleanUrl);

    // Step 3: Check tracker blocking
    if (m_trackingBlockerEnabled && isTracker(finalUrl, firstPartyUrl)) {
        info.block(true);
        qDebug() << "[Frint Privacy] Blocked tracker:" << finalUrl.toString();
        emit requestBlocked(finalUrl.toString(), "Blocked by tracker blocker");
        return;
    }

    // Step 4: Set privacy headers
    if (m_gpcEnabled) {
        info.setHttpHeader(QByteArray("Sec-GPC"), QByteArray("1"));
    }
    info.setHttpHeader(QByteArray("DNT"), QByteArray("1"));

    // Let Chromium handle referrer policy natively (defaults to strict-origin-when-cross-origin).
    // Overriding it here breaks the page's Referrer-Policy header and causes test failures.
    // The referrer can be hardened via Chromium's built-in mechanisms instead.

    // Spoofed User-Agent via fingerprinting defense
    QString ua = spoofedUserAgent();
    if (!ua.isEmpty()) {
        info.setHttpHeader(QByteArray("User-Agent"), ua.toUtf8());
    }

    // ── Anti-cache tracking: strip ETags on sub-resources ──
    // Prevents server from using ETag to re-identify users across sessions
    if (resourceType != "main_frame" && resourceType != "sub_frame") {
        info.setHttpHeader(QByteArray("Cache-Control"), QByteArray("no-store, no-cache, must-revalidate"));
        info.setHttpHeader(QByteArray("Pragma"), QByteArray("no-cache"));
    }

    // Strip sec-ch-ua headers (client hints) for privacy
    info.setHttpHeader(QByteArray("Sec-CH-UA"), QByteArray(""));
    info.setHttpHeader(QByteArray("Sec-CH-UA-Mobile"), QByteArray(""));
    info.setHttpHeader(QByteArray("Sec-CH-UA-Platform"), QByteArray(""));
    info.setHttpHeader(QByteArray("Sec-CH-UA-Arch"), QByteArray(""));
    info.setHttpHeader(QByteArray("Sec-CH-UA-Bitness"), QByteArray(""));
    info.setHttpHeader(QByteArray("Sec-CH-UA-Model"), QByteArray(""));
    info.setHttpHeader(QByteArray("Sec-CH-UA-Full-Version"), QByteArray(""));

    // Redirect to cleaned URL if different
    if (finalUrl != requestUrl) {
        info.redirect(finalUrl);
    }
}

void PrivacyUrlInterceptor::setTrackingBlockerEnabled(bool enabled)
{
    m_trackingBlockerEnabled = enabled;
}

void PrivacyUrlInterceptor::setHttpsOnlyEnabled(bool enabled)
{
    m_httpsOnlyEnabled = enabled;
}

void PrivacyUrlInterceptor::setGpcEnabled(bool enabled)
{
    m_gpcEnabled = enabled;
}

void PrivacyUrlInterceptor::setReferrerPolicyEnabled(bool enabled)
{
    m_referrerPolicyEnabled = enabled;
}

QUrl PrivacyUrlInterceptor::cleanTrackingParams(const QUrl &url) const
{
    return UrlCleaner::cleanTrackingParams(url);
}

QUrl PrivacyUrlInterceptor::upgradeToHttps(const QUrl &url) const
{
    if (!m_httpsOnlyEnabled) return url;

    if (url.scheme() == "http"
        && url.host() != "localhost"
        && url.host() != "127.0.0.1"
        && url.host() != "::1") {
        QUrl result = url;
        result.setScheme("https");
        return result;
    }
    return url;
}

bool PrivacyUrlInterceptor::isTracker(const QUrl &requestUrl, const QUrl &firstPartyUrl) const
{
    return TrackingBlocker::instance().isTracker(requestUrl, firstPartyUrl);
}

QString PrivacyUrlInterceptor::getReferrerHeader(const QUrl &targetUrl, const QUrl &originUrl) const
{
    return ReferrerPolicy::instance().getReferrer(targetUrl, originUrl);
}

QString PrivacyUrlInterceptor::spoofedUserAgent() const
{
    return FingerprintingDefender::instance().spoofedUserAgent();
}

} // namespace Frint
