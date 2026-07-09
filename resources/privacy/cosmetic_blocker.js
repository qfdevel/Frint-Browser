// ── Frint Cosmetic Ad-Blocking CSS Injection ──
(function() {
    var css = `
/* Ad containers */
div[id*="-ad"], div[id*="-ads"], div[id*="-banner"], div[id*="-sponsor"],
div[id*="-promo"], div[class*="-ad"], div[class*="-ads"],
div[class*="-banner"], div[class*="-sponsor"], div[class*="-promo"],
div[class*="ad-"], div[class*="ads-"], div[class*="banner-"],
aside[class*="ad"], aside[id*="ad"],
iframe[src*="doubleclick"], iframe[src*="googlesyndication"],
iframe[src*="amazon-adsystem"], iframe[src*="adservice"],
iframe[src*="adnxs"], iframe[src*="criteo"],
iframe[src*="taboola"], iframe[src*="outbrain"],
iframe[id*="google_ads"], iframe[id*="ad"],
/* Google Ads */
div[id^="google_ads"], ins.adsbygoogle,
/* Social widgets / tracking pixels */
iframe[src*="facebook.com/plugins"], iframe[src*="facebook.com/tr"],
img[src*="facebook.com/tr"], img[src*="pixel"],
img[src*="doubleclick"], img[src*="analytics"],
/* Common classes */
.ad-container, .ad-wrapper, .ad-slot, .ad-banner,
.advertisement, .advertising, .sponsored, .sponsor,
.promoted, .promotion, .announcement,
[aria-label="ad"], [aria-label="sponsor"],
/* Popups / overlays */
div[class*="popup"], div[class*="overlay"],
div[id*="popup"], div[id*="overlay"],
/* Specific platforms */
div[data-ad-target], div[data-google-query-id],
amp-ad, amp-embed, amp-sticky-ad,
/* Fixed position ads */
div[style*="position:fixed"][style*="bottom"],
div[style*="position: fixed"][style*="bottom"]
{ display: none !important; }

/* Prevent layout shift from hidden ads */
iframe[src*="doubleclick"], iframe[src*="adsystem"],
iframe[src*="googlesyndication"] { height: 0 !important; }
`;
    var s = document.createElement('style');
    s.textContent = css;
    document.head.appendChild(s);
})();
