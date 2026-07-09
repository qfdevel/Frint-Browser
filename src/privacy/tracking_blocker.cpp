#include "privacy/tracking_blocker.h"
#include <QDebug>

namespace Frint {

QStringList TrackingBlocker::s_knownTrackers = {
    // Google
    "doubleclick.net", "googlesyndication.com", "googletagmanager.com",
    "googleadservices.com", "google-analytics.com", "googletagservices.com",
    "adservice.google.com", "pagead2.googlesyndication.com",
    // Meta / Facebook
    "connect.facebook.net", "pixel.facebook.com",
    "an.facebook.com", "atlassbx.com",
    // Twitter / X
    "ads-twitter.com", "analytics.twitter.com",
    // General ad networks
    "adnxs.com", "adsrvr.org", "rubiconproject.com", "criteo.com",
    "criteo.net", "casalemedia.com", "moatads.com", "moat.com",
    "openx.net", "pubmatic.com", "bidswitch.net",
    "appnexus.com", "indexww.com", "agkn.com", "bluekai.com",
    "exelator.com", "demdex.net", "adsafeprotected.com",
    // Analytics
    "hotjar.com", "newrelic.com", "mouseflow.com", "fullstory.com",
    "crazyegg.com", "mixpanel.com", "amplitude.com",
    "segment.io", "segment.com", "clicky.com", "statcounter.com",
    "heap.com", "heapanalytics.com",
    // Ad serving
    "adzerk.net", "exponential.com", "sovrn.com", "sharethrough.com",
    "taboola.com", "outbrain.com", "revcontent.com",
    // LinkedIn
    "ads.linkedin.com", "linkedin.com",
    // TikTok
    "analytics.tiktok.com", "ads.tiktok.com", "pangle.io",
    // Microsoft / Bing
    "bat.bing.com", "c.bing.com",
    // Pinterest
    "analytics.pinterest.com", "ct.pinterest.com",
    // Reddit
    "events.reddit.com", "ads.reddit.com",
    // Snapchat
    "analytics.snapchat.com", "ads.snapchat.com",
    // Amazon
    "adsystem.amazon.com", "aax.amazon-adsystem.com",
    // Cloudflare tracking
    "cloudflareinsights.com"
};

TrackingBlocker &TrackingBlocker::instance()
{
    static TrackingBlocker s_instance;
    return s_instance;
}

TrackingBlocker::TrackingBlocker() = default;

bool TrackingBlocker::isTracker(const QUrl &requestUrl, const QUrl &pageUrl) const
{
    if (!m_enabled) {
        return false;
    }

    const QString requestHost = requestUrl.host().toLower();
    const QString pageHost = pageUrl.host().toLower();

    // Don't block same-origin requests
    if (requestHost == pageHost) {
        return false;
    }

    // Check known trackers (match host exactly or as subdomain)
    auto matchesDomain = [](const QString &host, const QString &tracker) -> bool {
        if (host == tracker) return true;
        if (host.endsWith("." + tracker)) return true;
        return false;
    };

    for (const QString &tracker : s_knownTrackers) {
        if (matchesDomain(requestHost, tracker)) {
            qDebug() << "[Frint] Blocked tracker:" << requestUrl.toString();
            return true;
        }
    }

    // Check custom domains
    for (const QString &custom : m_customDomains) {
        if (matchesDomain(requestHost, custom)) {
            qDebug() << "[Frint] Blocked custom tracker:" << requestUrl.toString();
            return true;
        }
    }

    return false;
}

void TrackingBlocker::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool TrackingBlocker::isEnabled() const
{
    return m_enabled;
}

QStringList TrackingBlocker::blockedDomains() const
{
    QStringList all = s_knownTrackers;
    all.append(m_customDomains);
    return all;
}

void TrackingBlocker::addCustomDomain(const QString &domain)
{
    if (!m_customDomains.contains(domain)) {
        m_customDomains.append(domain);
    }
}

void TrackingBlocker::removeCustomDomain(const QString &domain)
{
    m_customDomains.removeAll(domain);
}

} // namespace Frint
