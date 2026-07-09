#include "privacy/url_cleaner.h"
#include <QUrlQuery>
#include <QDebug>

namespace Frint {

// ── Tracking parameter lists (PrivacyTests.org compliant) ──────────────

QStringList UrlCleaner::s_trackingParams = {
    // Google Ads / Analytics
    "fbclid", "gclid", "gclsrc", "dclid", "gbraid", "wbraid",
    "msclkid", "twclid", "gad_source", "gad_campaign", "gad",
    "_ga", "_gl", "_gac", "gacid", "gclaw", "gclgb",

    // Meta / Facebook
    "fb_action_ids", "fb_action_types", "fb_source", "fb_ref",
    "fb_ad_group", "fb_ad_id", "fb_click_id", "fb_ad",
    "fbclid", "fbclid", "fbp", "fbc",

    // Twitter / X
    "twclid", "t.co",

    // UTM standard params (case-insensitive)
    "utm_source", "utm_medium", "utm_campaign", "utm_term",
    "utm_content", "utm_id", "utm_cid", "utm_reader",
    "utm_viz_id", "utm_pubreferrer", "utm_affiliate",
    "utm_social", "utm_zone", "utm_brand", "utm_vid",
    "utm_product", "utm_creative", "utm_p1", "utm_p2",
    "utm_p3", "utm_contact", "utm_company", "utm_location",
    "utm_network", "utm_target",

    // HubSpot / Marketo
    "mkt_tok", "hmb_campaign", "hmb_source", "hmb_medium",
    "_hsenc", "_hsmi", "__hstc", "__hsfp", "__hssc",
    "hsCtaTracking",

    // Mail tracking
    "mc_cid", "mc_eid", "ml_subscriber", "ml_subscriber_hash",
    "trk", "trkCampaign", "vero_conv", "vero_id",

    // Analytics platforms
    "pk_source", "pk_medium", "pk_campaign", "pk_keyword",
    "pk_content", "piwik_source", "piwik_medium",
    "piwik_campaign", "piwik_keyword",
    "_openstat", "yclid", "s_cid", "rb_clickid",
    "oly_anon_id", "oly_enc_id",

    // Misc tracking
    "__s", "wickedid", "wt_mc", "wt_zmc", "wt_zs", "wt_ni",
    "ICID", "ref_url", "ssp_iq", "ssp_imp_id",
    "zanpid", "soc_src", "soc_trk",
    "cmpid", "aff_id", "aff_sub", "aff_sub2", "aff_sub3",
    "aff_sub4", "aff_sub5",
    "et_rid", "et_attr", "lgt", "oly_enc_id", "oly_anon_id",
    "_openstat",
    "vero_conv", "vero_id", "mkt_tok",
    "mc_cid", "mc_eid", "email_hash",

    // Criteo / Ad serving
    "criteo_params", "criteo_id",

    // TikTok
    "tt_medium", "tt_content", "tt_campaign", "tt_keyword",

    // LinkedIn
    "li_fat_id",

    // Pinterest
    "pin_uv", "pi_ad_id", "pi_campaign_id",

    // Outbrain / Taboola
    "ob_origin", "ob_ref", "ob_click_id",
    "tb_click_id", "tb_source",

    // Taboola
    "utm_taboola",
};

QStringList UrlCleaner::s_utmVariants = {
    "utm_source", "utm_medium", "utm_campaign", "utm_term",
    "utm_content", "utm_id", "utm_cid", "utm_reader",
    "utm_viz_id", "utm_pubreferrer", "utm_affiliate",
    "utm_social", "utm_zone", "utm_brand", "utm_vid",
    "utm_product", "utm_creative", "utm_p1", "utm_p2",
    "utm_p3", "utm_contact", "utm_company", "utm_location",
    "utm_network", "utm_target",
};

QStringList UrlCleaner::s_adPlatformParams = {
    // Microsoft / Bing
    "msclkid",

    // Yahoo
    "yclid",

    // Yandex
    "yclid", "yclid",

    // Amazon
    "ref_", "pf_rd_p", "pf_rd_r", "pf_rd_s", "pf_rd_t",
    "pf_rd_i", "pd_rd_w", "pd_rd_r", "pd_rd_wg",

    // eBay
    "itm", "ssPageName", "epid",

    // Booking / Travel
    "aid", "label", "sid",
};

QUrl UrlCleaner::cleanTrackingParams(const QUrl &url)
{
    if (!url.hasQuery()) {
        return url;
    }

    QUrl cleaned = url;
    QUrlQuery query(url);

    QList<QPair<QString, QString>> cleanItems;
    int removedCount = 0;

    for (const auto &pair : query.queryItems()) {
        if (isTrackingParam(pair.first)) {
            removedCount++;
            continue;
        }
        cleanItems.append(pair);
    }

    if (removedCount == 0) {
        return url; // Nothing changed
    }

    QUrlQuery cleanQuery;
    cleanQuery.setQueryItems(cleanItems);
    cleaned.setQuery(cleanQuery);

    qDebug() << "[Frint UrlCleaner] Removed" << removedCount
             << "tracking params from" << url.toString()
             << "→" << cleaned.toString();

    return cleaned;
}

bool UrlCleaner::isTrackingParam(const QString &paramName)
{
    QString lower = paramName.toLower();

    // Check main list
    if (s_trackingParams.contains(lower)) {
        return true;
    }

    // Check UTM prefix
    if (lower.startsWith("utm_")) {
        return true;
    }

    // Check common ad prefixes
    if (lower.startsWith("fb_") || lower.startsWith("gcl")) {
        return true;
    }

    // Check HubSpot patterns
    if (lower.startsWith("_hs") || lower.startsWith("__h")) {
        return true;
    }

    // Check Piwik/Matomo patterns
    if (lower.startsWith("pk_") || lower.startsWith("piwik_")) {
        return true;
    }

    return false;
}

QStringList UrlCleaner::knownTrackingParams()
{
    QStringList all = s_trackingParams;

    // Add all UTM variants
    for (const auto &utm : s_utmVariants) {
        if (!all.contains(utm)) {
            all.append(utm);
        }
    }

    // Add all ad platform params
    for (const auto &ad : s_adPlatformParams) {
        if (!all.contains(ad)) {
            all.append(ad);
        }
    }

    all.sort();
    return all;
}

} // namespace Frint
