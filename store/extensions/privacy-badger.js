// Frint Browser - Privacy Badger Extension
// Learn and block invisible trackers

(function() {
    'use strict';

    class PrivacyBadger {
        constructor() {
            this.trackers = new Map();
            this.cookieBlocklist = new Set();
            this.learning = true;
        }

        // Analyze a third-party request
        analyzeRequest(url, pageUrl) {
            try {
                const urlObj = new URL(url);
                const pageObj = new URL(pageUrl);

                if (urlObj.hostname === pageObj.hostname) return;

                const domain = urlObj.hostname;
                if (!this.trackers.has(domain)) {
                    this.trackers.set(domain, {
                        count: 0,
                        cookies: false,
                        localStorage: false,
                        canvas: false,
                        fingerprint: false,
                        firstSeen: new Date().toISOString(),
                        actions: []
                    });
                }

                const tracker = this.trackers.get(domain);
                tracker.count++;

                // Check for cookies
                if (document.cookie && url.includes(domain)) {
                    tracker.cookies = true;
                }

                // Check for localStorage attempts
                try {
                    if (localStorage.length > 0) {
                        tracker.localStorage = true;
                    }
                } catch(e) {}

                // Determine action
                if (tracker.count >= 3) {
                    if (!tracker.actions.includes('block')) {
                        tracker.actions.push('block');
                        this.cookieBlocklist.add(domain);
                        console.log('[Privacy Badger] Blocking tracker:', domain);
                    }
                } else if (tracker.count >= 1) {
                    if (!tracker.actions.includes('cookie-block')) {
                        tracker.actions.push('cookie-block');
                        console.log('[Privacy Badger] Cookie-blocking:', domain);
                    }
                }
            } catch(e) {
                // Ignore URL parsing errors
            }
        }

        // Get tracking report
        getReport() {
            const report = [];
            for (const [domain, data] of this.trackers) {
                report.push({
                    domain: domain,
                    requests: data.count,
                    usesCookies: data.cookies,
                    usesStorage: data.localStorage,
                    actions: data.actions,
                    firstSeen: data.firstSeen
                });
            }
            report.sort((a, b) => b.requests - a.requests);
            return report;
        }

        // Get blocked domains
        getBlockedDomains() {
            return Array.from(this.cookieBlocklist);
        }

        // Clear all data
        reset() {
            this.trackers.clear();
            this.cookieBlocklist.clear();
            console.log('[Privacy Badger] Reset complete');
        }
    }

    const badger = new PrivacyBadger();
    window.__privacyBadger = badger;

    // Monitor network requests for third-party tracking
    const originalXHROpen = XMLHttpRequest.prototype.open;
    XMLHttpRequest.prototype.open = function(method, url) {
        try {
            badger.analyzeRequest(url.toString(), window.location.href);
        } catch(e) {}
        return originalXHROpen.apply(this, arguments);
    };

    const originalFetch = window.fetch;
    window.fetch = function(input, init) {
        try {
            const url = typeof input === 'string' ? input : input.url;
            badger.analyzeRequest(url, window.location.href);
        } catch(e) {}
        return originalFetch.apply(this, arguments);
    };

    // Periodic reporting
    setInterval(() => {
        const blocked = badger.getBlockedDomains();
        if (blocked.length > 0) {
            console.log('[Privacy Badger] Active blocks:', blocked.length);
        }
    }, 30000);

    console.log('[Privacy Badger] Initialized');
})();
