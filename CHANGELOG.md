# Changelog

All notable changes to Frint Browser are documented here.

## [0.1.0-alpha] — 2026-07-09

### Added

#### Core Browser
- Qt6-based browser application with tabbed interface
- Address bar with DuckDuckGo search fallback
- Navigation controls (back, forward, reload)
- Status bar with privacy indicator
- Tab management (new, close, reorder, move)
- Command line argument parsing (URL, help, version)
- Settings persistence via QSettings (INI format)

#### Privacy Modules
- **UrlCleaner** — Strips 200+ tracking query parameters
  - UTM parameters (all variants)
  - Google Ads (fbclid, gclid, gclsrc, dclid, gbraid, wbraid)
  - Facebook/Meta (fb_action_ids, fb_click_id, fbc, fbp)
  - HubSpot/Marketo (mkt_tok, __hstc, hsCtaTracking)
  - Mail tracking (mc_cid, mc_eid, ml_subscriber)
  - Analytics platforms (pk_*, piwik_*, oly_enc_id)
  - Ad platforms (msclkid, yclid, ref_*, pf_rd_*)
  - Matomo, Piwik, Criteo, TikTok, LinkedIn, Pinterest params
  - Case-insensitive matching

- **ReferrerPolicy** — Strict-origin-when-cross-origin enforcement
  - Full URL for same-origin requests
  - Origin only for cross-origin same-scheme requests
  - No referrer on HTTPS→HTTP downgrade
  - Configurable policy level

- **TrackingBlocker** — Blocks 150+ known tracking domains
  - Google (doubleclick.net, google-analytics.com, googletagmanager.com)
  - Facebook/Meta (connect.facebook.net, pixel.facebook.com)
  - Amazon, Microsoft/Bing, Adobe, AppNexus, Criteo
  - Taboola, Outbrain, LinkedIn, TikTok, Pinterest, Reddit
  - Analytics (Hotjar, NewRelic, FullStory, Mixpanel, Segment)
  - Ad serving (OpenX, PubMatic, Rubicon, Casale)
  - Same-origin bypass, custom domain support
  - Tracking cookie blocking

- **FingerprintingDefender** — Multi-vector fingerprinting defense
  - Font enumeration spoofing (20 fixed fonts)
  - Canvas fingerprinting (fixed hash return)
  - Screen resolution spoofing (1920×1080 fixed)
  - Timezone spoofing (UTC rounded)
  - Language spoofing (en-US default)
  - Hardware concurrency spoofing (4 cores)
  - Device memory spoofing (8 GB)
  - User-Agent spoofing (Chrome-based generic)

- **DnsResolver** — DNS-over-HTTPS with Cloudflare
  - Async and blocking resolution
  - Configurable endpoints (Cloudflare, Quad9, Google)
  - DNS result caching (5 min TTL, 256 entries)
  - Graceful fallback on error

- **GpcHeader** — Global Privacy Control
  - Sends `Sec-GPC: 1` with all HTTP requests
  - Toggleable at runtime

#### Storage
- **StoragePartition** — Origin-isolated storage
  - SHA-256 hashed partition IDs
  - Per-origin directory isolation
  - Clear all / clear per origin
  - Configurable base path

#### Build System
- CMake 3.20+, C++20, Qt6
- Security hardening flags
- Optional Ladybird LibWeb integration flag
- Ninja and Make support
- Automated config file copying

#### Plugin/Theme Store
- Store manifest with 3 themes and 3 extensions
- JSON validation script
- GitHub Actions workflow for PR validation
- Theme color schemas (Dark, Light, Nord)
- Extension manifests (uBlock Origin, NoScript, Privacy Badger compat)

#### Documentation
- Comprehensive README.md
- CONTRIBUTING.md with guidelines
- CHANGELOG.md
- Store README.md
- PrivacyTests.org compliance table

#### Security
- `-D_FORTIFY_SOURCE=2` compile flag
- `-fstack-protector-strong` and `-fPIE`
- `-Wl,-z,relro -Wl,-z,now` link flags
- `FRINT_SECURE_DEFAULTS` preprocessor define
- All privacy features enabled by default in `default_prefs.json`
