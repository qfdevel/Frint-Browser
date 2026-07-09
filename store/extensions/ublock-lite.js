// Frint Browser - uBlock Lite Extension
// Lightweight ad and tracker blocker

(function() {
    'use strict';

    const FILTERS = [
        '*doubleclick.net*',
        '*googlesyndication.com*',
        '*googleadservices.com*',
        '*google-analytics.com*',
        '*googletagmanager.com*',
        '*facebook.com/tr*',
        '*amazon-adsystem.com*',
        '*adsrvr.org*',
        '*adnxs.com*',
        '*rubiconproject.com*',
        '*criteo.com*',
        '*criteo.net*',
        '*casalemedia.com*',
        '*moatads.com*',
        '*outbrain.com*',
        '*taboola.com*',
        '*scorecardresearch.com*',
        '*quantserve.com*',
        '*exelator.com*',
        '*demdex.net*',
        '*adsafeprotected.com*',
        '*adzerk.net*',
        '*ads-twitter.com*',
        '*advertising.com*',
        '*tremorhub.com*',
        '*pubmatic.com*',
        '*openx.net*',
        '*appnexus.com*',
        '*contextweb.com*',
        '*sharethrough.com*',
        '*indexww.com*',
        '*sovrn.com*',
        '*bidswitch.net*',
        '*agkn.com*',
        '*media.net*',
        '*adroll.com*',
        '*optimizely.com*',
        '*hotjar.com*',
        '*crazyegg.com*'
    ];

    class UBlockLite {
        constructor() {
            this.blockedCount = 0;
            this.blockedRequests = [];
        }

        shouldBlock(url) {
            for (const filter of FILTERS) {
                const pattern = filter.replace(/\*/g, '.*');
                const regex = new RegExp(pattern);
                if (regex.test(url)) {
                    return true;
                }
            }
            return false;
        }

        blockRequest(url) {
            this.blockedCount++;
            this.blockedRequests.push({
                url: url,
                time: new Date().toISOString()
            });

            console.log('[uBlock Lite] Blocked:', url);
            console.log('[uBlock Lite] Total blocked:', this.blockedCount);

            return true;
        }

        getStats() {
            return {
                blockedCount: this.blockedCount,
                blockedRequests: this.blockedRequests.slice(-100)
            };
        }
    }

    const ublock = new UBlockLite();
    window.__ublockLite = ublock;

    // Intercept network requests
    const originalFetch = window.fetch;
    window.fetch = function(input, init) {
        const url = typeof input === 'string' ? input : input.url;
        if (ublock.shouldBlock(url)) {
            ublock.blockRequest(url);
            return Promise.reject(new Error('Blocked by uBlock Lite'));
        }
        return originalFetch.apply(this, arguments);
    };

    // Patch XMLHttpRequest
    const originalOpen = XMLHttpRequest.prototype.open;
    XMLHttpRequest.prototype.open = function(method, url) {
        if (ublock.shouldBlock(url.toString())) {
            ublock.blockRequest(url.toString());
            console.warn('[uBlock Lite] Blocked XHR:', url);
            this.abort();
            return;
        }
        return originalOpen.apply(this, arguments);
    };
})();
