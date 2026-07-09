#include "privacy/url_cleaner.h"
#include <QUrlQuery>

namespace Frint {

const QStringList UrlCleaner::s_trackingParams = {
    // Google Ads / Analytics
    "fbclid", "gclid", "gclsrc", "dclid", "gbraid", "wbraid",
    "msclkid", "twclid", "gad_source", "gad_campaign",
    // UTM parameters
    "utm_source", "utm_medium", "utm_campaign", "utm_term",
    "utm_content", "utm_id", "utm_cid", "utm_reader", "utm_viz_id",
    "utm_pubreferrer", "utm_affiliate", "utm_social", "_openstat",
    // Meta / Facebook
    "fb_action_ids", "fb_action_types", "fb_source", "fb_ref",
    "fb_ad_group", "fb_ad_id", "fb_click_id", "fb_ad",
    // Twitter / X
    "t.co", "twclid",
    // HubSpot / Marketo
    "mkt_tok", "hmb_campaign", "hmb_source", "hmb_medium",
    // Misc tracking
    "yclid", "_ga", "_gl", "mc_cid", "mc_eid",
    "trk", "trkCampaign", "vero_conv", "vero_id",
    "pk_source", "pk_medium", "pk_campaign", "pk_keyword",
    "pk_content", "piwik_source", "piwik_medium", "piwik_campaign",
    "piwik_keyword", "_hsenc", "_hsmi", "__hstc", "__hsfp", "__hssc",
    "hsCtaTracking", "oly_anon_id", "oly_enc_id",
    "wickedid", "wt_mc", "wt_zmc", "wt_zs", "wt_ni",
    "ICID", "ref_url", "ssp_iq", "ssp_imp_id",
    "zanpid", "soc_src", "soc_trk", "vero_conv", "vero_id",
    "cmpid", "aff_id", "aff_sub", "aff_sub2", "aff_sub3",
    "aff_sub4", "aff_sub5"
};

QUrl UrlCleaner::cleanTrackingParams(const QUrl &url)
{
    if (!url.hasQuery()) {
        return url;
    }

    QUrl cleaned = url;
    QUrlQuery query(url);

    // Remove known tracking parameters
    QStringList keepParams;
    const auto items = query.queryItems();
    for (const auto &pair : items) {
        if (!s_trackingParams.contains(pair.first, Qt::CaseInsensitive)) {
            keepParams.append(pair.first + "=" + pair.second);
        }
    }

    if (keepParams.size() == query.queryItems().size()) {
        return url; // Nothing changed
    }

    cleaned.setQuery(keepParams.join("&"));
    return cleaned;
}

} // namespace Frint
