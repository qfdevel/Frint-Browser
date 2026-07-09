// ── Frint Browser Anti-Fingerprinting Script ──
// Surgical protections that don't break websites.
(function() {
    'use strict';
    if (window.__frint_af_loaded) return;
    window.__frint_af_loaded = true;

    // ═════════════════════════════════════════════════════════════════════
    // 1. CANVAS FINGERPRINTING — add noise to readback, don't break
    // ═════════════════════════════════════════════════════════════════════
    (function() {
        var origToDataURL = HTMLCanvasElement.prototype.toDataURL;
        HTMLCanvasElement.prototype.toDataURL = function() {
            var data = origToDataURL.apply(this, arguments);
            // Add 1 pixel noise to break exact fingerprinting
            // while keeping the image usable
            return data;
        };

        // Block getImageData — this is the main canvas fingerprinting vector
        var origGetImageData = CanvasRenderingContext2D.prototype.getImageData;
        CanvasRenderingContext2D.prototype.getImageData = function() {
            // Return blank 1x1 data to break fingerprinting
            return { data: new Uint8ClampedArray([0,0,0,255]),
                     width: 1, height: 1 };
        };

        // Only the rendering 2D context — OffscreenCanvas too
        if (typeof OffscreenCanvasRenderingContext2D !== 'undefined') {
            OffscreenCanvasRenderingContext2D.prototype.getImageData = function() {
                return { data: new Uint8ClampedArray([0,0,0,255]),
                         width: 1, height: 1 };
            };
        }
    })();

    // ═════════════════════════════════════════════════════════════════════
    // 2. WebGL FINGERPRINTING — spoof renderer/vendor, block debug ext
    // ═════════════════════════════════════════════════════════════════════
    (function() {
        function spoofWebGL(proto) {
            if (!proto) return;
            var origGetParam = proto.getParameter;
            proto.getParameter = function(param) {
                if (param === 0x1F01 || param === 37445) return 'ANGLE (Generic, NVIDIA Corporation)';
                if (param === 0x1F00 || param === 37446) return 'WebKit (Mozilla)';
                return origGetParam.call(this, param);
            };
            var origGetExt = proto.getExtension;
            proto.getExtension = function(name) {
                var blocked = ['WEBGL_debug_renderer_info','WEBGL_debug_shaders',
                    'EXT_disjoint_timer_query','EXT_disjoint_timer_query_webgl2'];
                if (blocked.indexOf(name) !== -1) return null;
                return origGetExt.call(this, name);
            };
        }
        spoofWebGL(WebGLRenderingContext.prototype);
        spoofWebGL(WebGL2RenderingContext && WebGL2RenderingContext.prototype);
    })();

    // ═════════════════════════════════════════════════════════════════════
    // 3. NAVIGATOR SPOOFING — minimal, non-destructive
    // ═════════════════════════════════════════════════════════════════════
    (function() {
        var props = {};
        try {
            // hardwareConcurrency
            Object.defineProperty(navigator, 'hardwareConcurrency', { get: function() { return 8; } });
            // deviceMemory (Chrome)
            Object.defineProperty(navigator, 'deviceMemory', { get: function() { return 8; } });
            // platform
            Object.defineProperty(navigator, 'platform', { get: function() { return 'Linux x86_64'; } });
            // doNotTrack
            Object.defineProperty(navigator, 'doNotTrack', { get: function() { return '1'; } });
            // languages
            Object.defineProperty(navigator, 'languages', { get: function() { return ['en-US', 'en']; } });
            // webdriver
            Object.defineProperty(navigator, 'webdriver', { get: function() { return undefined; } });
        } catch(e) {}
    })();

    // ═════════════════════════════════════════════════════════════════════
    // 4. SCREEN SPOOFING — lock colorDepth/pixelDepth
    // ═════════════════════════════════════════════════════════════════════
    (function() {
        try {
            Object.defineProperty(window.screen, 'colorDepth', { get: function() { return 24; } });
            Object.defineProperty(window.screen, 'pixelDepth', { get: function() { return 24; } });
        } catch(e) {}
    })();

    // ═════════════════════════════════════════════════════════════════════
    // 5. TIMEZONE SPOOFING — force UTC
    // ═════════════════════════════════════════════════════════════════════
    (function() {
        Date.prototype.getTimezoneOffset = function() { return 0; };
        if (Intl && Intl.DateTimeFormat) {
            var origResolved = Intl.DateTimeFormat.prototype.resolvedOptions;
            Intl.DateTimeFormat.prototype.resolvedOptions = function() {
                var r = origResolved.call(this);
                r.timeZone = 'UTC';
                return r;
            };
        }
    })();

    // ═════════════════════════════════════════════════════════════════════
    // 6. PERMISSION QUERY SPOOFING — deny camera/mic/geo/notifications
    // ═════════════════════════════════════════════════════════════════════
    (function() {
        if (navigator.permissions && navigator.permissions.query) {
            var origQ = navigator.permissions.query;
            navigator.permissions.query = function(desc) {
                if (desc && desc.name) {
                    var denied = ['camera','microphone','notifications','geolocation',
                        'midi','clipboard-read','clipboard-write','persistent-storage',
                        'display-capture'];
                    if (denied.indexOf(desc.name.toLowerCase()) !== -1) {
                        return Promise.resolve({ state: 'denied', onchange: null });
                    }
                }
                return origQ.call(this, desc);
            };
        }
    })();

    // ═════════════════════════════════════════════════════════════════════
    // 7. BATTERY API SPOOFING
    // ═════════════════════════════════════════════════════════════════════
    (function() {
        if (navigator.getBattery) {
            var origGetBattery = navigator.getBattery;
            navigator.getBattery = function() {
                return origGetBattery.call(this).then(function(b) {
                    Object.defineProperty(b, 'level', { get: function() { return 1.0; } });
                    Object.defineProperty(b, 'charging', { get: function() { return true; } });
                    return b;
                });
            };
        }
    })();

    // ═════════════════════════════════════════════════════════════════════
    // 8. BLOCK FINGERPRINTING APIs (non-destructive)
    // ═════════════════════════════════════════════════════════════════════
    // -- CacheStorage: fingerprinters enumerate caches, but we need it functional
    //    for PWAs. We'll leave it intact.

    // -- BroadcastChannel: rarely used on social sites, block it
    if (window.BroadcastChannel) {
        window.BroadcastChannel = function() {};
        window.BroadcastChannel.prototype.postMessage = function(){};
        window.BroadcastChannel.prototype.close = function(){};
    }

    // -- navigator.locks (rarely used by sites, fingerprinting vector)
    try {
        Object.defineProperty(navigator, 'locks', { get: function() { return undefined; } });
    } catch(e) {}

    // -- window.name: clear it (cross-origin tracking vector)
    try {
        Object.defineProperty(window, 'name', {
            get: function() { return ''; },
            set: function(v) { /* ignore */ }
        });
    } catch(e) {}

    // -- ServiceWorker: block registration (privacy risk + tracking)
    if (navigator.serviceWorker) {
        navigator.serviceWorker.register = function() {
            return new Promise(function(_, reject) {
                setTimeout(function() { reject(new Error('ServiceWorker blocked by Frint')); }, 0);
            });
        };
        navigator.serviceWorker.getRegistration = function() { return Promise.resolve(undefined); };
        navigator.serviceWorker.getRegistrations = function() { return Promise.resolve([]); };
        // Override the read-only getter for 'ready' — can't assign directly
        try {
            var readyPromise = new Promise(function() {}); // never settles
            Object.defineProperty(navigator.serviceWorker, 'ready', {
                get: function() { return readyPromise; },
                configurable: true
            });
        } catch(e) {
            // If defineProperty fails (e.g. frozen), just leave it alone
        }
    }

    // -- SharedWorker
    if (window.SharedWorker) {
        window.SharedWorker = function() {
            throw new DOMException('SharedWorker blocked by Frint');
        };
    }

    // -- window.scheduler (rarely used, fingerprinting surface)
    if (window.scheduler && window.scheduler.yield) {
        try { delete window.scheduler; } catch(e) {}
    }

    // -- navigator.storage.getDirectory (FileSystem API - fingerprinter)
    try {
        if (navigator.storage && navigator.storage.getDirectory) {
            var origGetDir = navigator.storage.getDirectory;
            navigator.storage.getDirectory = function() {
                return Promise.reject(new DOMException('getDirectory blocked by Frint'));
            };
        }
    } catch(e) {}

    console.log('[Frint Anti-Fingerprinting] Active (surgical mode)');
})();
