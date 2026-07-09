#include "privacy/tracking_blocker.h"
#include <QDebug>

namespace Frint {

// ── Known tracking domains (PrivacyTests.org + Disconnect list based) ──

QStringList TrackingBlocker::s_knownTrackers = {
    // ── Google ─────────────────────────────────────────────────────────
    "doubleclick.net",
    "googleads.g.doubleclick.net",
    "googlesyndication.com",
    "googletagmanager.com",
    "googleadservices.com",
    "google-analytics.com",
    "googletagservices.com",
    "adservice.google.com",
    "pagead2.googlesyndication.com",
    "adservice.google.co.uk",
    "adservice.google.de",
    "adservice.google.fr",
    "adservice.google.ca",
    "adservice.google.co.jp",
    "adservice.google.com.au",
    "adservice.google.it",
    "adservice.google.es",
    "adservice.google.nl",
    "adservice.google.com.br",
    "adservice.google.com.mx",
    "partner.googleadservices.com",
    "www.googletagmanager.com",
    "region1.google-analytics.com",
    "region1.google-analytics.l.google.com",
    "stats.g.doubleclick.net",
    "td.doubleclick.net",
    "ad.doubleclick.net",
    "cm.g.doubleclick.net",
    "securepubads.g.doubleclick.net",

    // ── Facebook / Meta ────────────────────────────────────────────────
    "facebook.com/tr",
    "connect.facebook.net",
    "pixel.facebook.com",
    "an.facebook.com",
    "static.xx.fbcdn.net",
    "pixel.quantserve.com",
    "atlassbx.com",
    "facebook.net",

    // ── Twitter / X ────────────────────────────────────────────────────
    "analytics.twitter.com",
    "ads-twitter.com",
    "t.co",

    // ── Amazon ─────────────────────────────────────────────────────────
    "amazon-adsystem.com",
    "aax.amazon-adsystem.com",
    "amazonadsi.com",
    "rcm-na.amazon-adsystem.com",

    // ── Microsoft / Bing ───────────────────────────────────────────────
    "bat.bing.com",
    "c.bing.com",
    "adserver.microsoft.com",
    "ads.bing.com",

    // ── Adobe ──────────────────────────────────────────────────────────
    "adobe.com",
    "adobedtm.com",
    "adobeecm.com",
    "demdex.net",
    "dpm.demdex.net",
    "fast.thirdparty.com",

    // ── AppNexus / Xandr ──────────────────────────────────────────────
    "adnxs.com",
    "appnexus.com",
    "ib.adnxs.com",
    "secure.adnxs.com",

    // ── Criteo ─────────────────────────────────────────────────────────
    "criteo.com",
    "criteo.net",
    "static.criteo.net",
    "dis.criteo.com",
    "cas.criteo.com",
    "sslwidget.criteo.com",

    // ── Taboola ────────────────────────────────────────────────────────
    "taboola.com",
    "trc.taboola.com",
    "cdn.taboola.com",
    "vidstat.taboola.com",

    // ── Outbrain ───────────────────────────────────────────────────────
    "outbrain.com",
    "odb.outbrain.com",
    "widgets.outbrain.com",
    "amplify.outbrain.com",

    // ── LinkedIn ───────────────────────────────────────────────────────
    "ads.linkedin.com",
    "linkedin.com/analytics",

    // ── TikTok ─────────────────────────────────────────────────────────
    "analytics.tiktok.com",
    "ads.tiktok.com",
    "pangle.io",

    // ── Pinterest ──────────────────────────────────────────────────────
    "analytics.pinterest.com",
    "ct.pinterest.com",
    "ads.pinterest.com",

    // ── Reddit ─────────────────────────────────────────────────────────
    "events.reddit.com",
    "ads.reddit.com",

    // ── Snapchat ───────────────────────────────────────────────────────
    "analytics.snapchat.com",
    "ads.snapchat.com",

    // ── Yahoo / Verizon ───────────────────────────────────────────────
    "yahoo.com",
    "adserver.yahoo.com",
    "analytics.yahoo.com",
    "yimg.com",

    // ── Yandex ─────────────────────────────────────────────────────────
    "yandex.com",
    "yandex.ru",
    "mc.yandex.ru",
    "mc.yandex.com",

    // ── General ad / analytics networks ───────────────────────────────
    "adsrvr.org",
    "rubiconproject.com",
    "casalemedia.com",
    "moatads.com",
    "moat.com",
    "openx.net",
    "pubmatic.com",
    "bidswitch.net",
    "indexww.com",
    "agkn.com",
    "bluekai.com",
    "exelator.com",
    "adsafeprotected.com",
    "scorecardresearch.com",
    "comscore.com",
    "comscoreresearch.com",

    // ── Analytics platforms ────────────────────────────────────────────
    "hotjar.com",
    "newrelic.com",
    "mouseflow.com",
    "fullstory.com",
    "crazyegg.com",
    "mixpanel.com",
    "amplitude.com",
    "segment.io",
    "segment.com",
    "clicky.com",
    "statcounter.com",
    "heap.com",
    "heapanalytics.com",
    "quantcast.com",
    "chartbeat.com",
    "chartbeat.net",
    "ping.chartbeat.net",

    // ── Ad serving ─────────────────────────────────────────────────────
    "adzerk.net",
    "exponential.com",
    "sovrn.com",
    "sharethrough.com",
    "revcontent.com",

    // ── Cloudflare analytics ──────────────────────────────────────────
    "cloudflareinsights.com",

    // ── Other trackers ─────────────────────────────────────────────────
    "quantserve.com",
    "quantserve.net",
    "prosperent.com",
    "viglink.com",
    "skimresources.com",
    "linksynergy.com",
    "affiliate.com",
    "shareasale.com",
    "cj.com",
    "awin.com",
};

TrackingBlocker &TrackingBlocker::instance()
{
    static TrackingBlocker s_instance;
    return s_instance;
}

TrackingBlocker::TrackingBlocker() = default;

void TrackingBlocker::setEnabled(bool enabled)
{
    m_enabled = enabled;
    qDebug() << "[Frint TrackingBlocker]" << (enabled ? "Enabled" : "Disabled");
}

bool TrackingBlocker::isEnabled() const
{
    return m_enabled;
}

bool TrackingBlocker::isTracker(const QUrl &requestUrl, const QUrl &pageUrl) const
{
    if (!m_enabled) {
        return false;
    }

    const QString requestHost = requestUrl.host().toLower();
    const QString pageHost = pageUrl.host().toLower();

    // Never block same-origin
    if (requestHost == pageHost) {
        return false;
    }

    // Matching helper: exact match or subdomain match
    auto matches = [](const QString &host, const QString &tracker) -> bool {
        if (host == tracker) return true;
        if (host.endsWith("." + tracker)) return true;
        return false;
    };

    // Check known trackers
    for (const QString &tracker : s_knownTrackers) {
        // Handle trackers with path component (e.g. "facebook.com/tr")
        if (tracker.contains('/')) {
            QString domain = tracker.section('/', 0, 0);
            QString path = "/" + tracker.section('/', 1);
            if (requestHost.contains(domain) || matches(requestHost, domain)) {
                if (requestUrl.path().startsWith(path)) {
                    qDebug() << "[Frint TrackingBlocker] Blocked tracker:"
                             << requestUrl.toString();
                    return true;
                }
            }
        } else if (matches(requestHost, tracker)) {
            qDebug() << "[Frint TrackingBlocker] Blocked tracker:"
                     << requestUrl.toString();
            return true;
        }
    }

    // Check custom domains
    for (const QString &custom : m_customDomains) {
        if (matches(requestHost, custom)) {
            qDebug() << "[Frint TrackingBlocker] Blocked custom tracker:"
                     << requestUrl.toString();
            return true;
        }
    }

    return false;
}

bool TrackingBlocker::isTrackingCookie(const QString &cookieDomain,
                                       const QUrl &pageUrl) const
{
    if (!m_enabled) {
        return false;
    }

    const QString pageHost = pageUrl.host().toLower();
    const QString cookieHost = cookieDomain.toLower();

    // Same-origin cookies are allowed
    if (cookieHost == pageHost || pageHost.endsWith("." + cookieHost)) {
        return false;
    }

    // Check if this is a known tracking domain
    auto matches = [](const QString &host, const QString &tracker) -> bool {
        if (host == tracker) return true;
        if (host.endsWith("." + tracker)) return true;
        return false;
    };

    for (const QString &tracker : s_knownTrackers) {
        // Only check pure domain trackers for cookies
        if (!tracker.contains('/') && matches(cookieHost, tracker)) {
            qDebug() << "[Frint TrackingBlocker] Blocked tracking cookie from:"
                     << cookieHost;
            return true;
        }
    }

    return false;
}

void TrackingBlocker::addCustomDomain(const QString &domain)
{
    QString clean = domain.toLower().trimmed();
    if (!m_customDomains.contains(clean)) {
        m_customDomains.append(clean);
        qDebug() << "[Frint TrackingBlocker] Added custom domain:" << clean;
    }
}

void TrackingBlocker::removeCustomDomain(const QString &domain)
{
    QString clean = domain.toLower().trimmed();
    m_customDomains.removeAll(clean);
    qDebug() << "[Frint TrackingBlocker] Removed custom domain:" << clean;
}

QStringList TrackingBlocker::blockedDomains() const
{
    QStringList all = s_knownTrackers;

    // Extract pure domains (remove path suffixes)
    QStringList pureDomains;
    for (const QString &d : all) {
        if (!d.contains('/')) {
            pureDomains.append(d);
        } else {
            pureDomains.append(d.section('/', 0, 0));
        }
    }

    // Remove duplicates
    QSet<QString> unique(pureDomains.begin(), pureDomains.end());
    pureDomains = unique.values();
    pureDomains.sort();

    pureDomains.append(m_customDomains);
    return pureDomains;
}

} // namespace Frint
