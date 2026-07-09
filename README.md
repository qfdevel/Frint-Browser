# Frint Browser 🛡️

**Privacy-first. Telemetry-free. Yours.**

Frint is a privacy-focused web browser built on Qt6 WebEngine, with a comprehensive privacy engine that blocks trackers, ads, and fingerprinting — right out of the box.

## Features

### 🔒 Privacy Engine
- **Tracker Blocker** — Blocks 200+ known tracking domains (Google Ads, Facebook/Meta, DoubleClick, Criteo, Taboola, and more)
- **URL Parameter Cleaner** — Strips 80+ tracking parameters from URLs (`fbclid`, `gclid`, `utm_*`, `_hsenc`, and more)
- **GPC (Global Privacy Control)** — Sends `Sec-GPC: 1` and `DNT: 1` headers on every request
- **HTTPS-Only Mode** — Upgrades all `http://` connections to `https://`
- **DNS-over-HTTPS (DoH)** — Encrypted DNS via Cloudflare by default
- **Anti-Fingerprinting** — JavaScript injection spoofs Canvas, WebGL, AudioContext, fonts, timezone, hardware info, and more
- **WebRTC IP Leak Protection** — Blocks real IP discovery via WebRTC
- **ETag & Cache Tracking Protection** — Strips cache-based tracking identifiers
- **Client Hints Blocking** — Removes `Sec-CH-UA-*` headers
- **Permission Auto-Deny** — All camera, mic, geolocation, and notification requests are denied by default
- **No Persistent Cookies** — Session-only cookies, wiped on exit

### 🎨 Liquid Glass UI
- Glassmorphism design with semi-transparent elements
- Three dark themes (Catppuccin Mocha, Nord Dark, Gruvbox Material)
- Custom CSS overrides
- Smooth entrance animation

### 📦 Built-in Extension Support
- Dark Reader
- uBlock Origin Lite (compat)
- Privacy Badger (compat)

### 🗂️ Profile Management
- Isolated browsing profiles with separate data
- Quick profile switching
- Complete data deletion

## Screenshots

### Google
<img width="1920" height="1025" alt="image" src="https://github.com/user-attachments/assets/dccb1e7b-fd06-4a40-9304-a9830896fbc7" />

### Selecting a theme
<img width="1920" height="1025" alt="image" src="https://github.com/user-attachments/assets/302873b9-198c-4f76-9207-53f857d58572" />

### RAM usage with 2 Gemini AI windows open
<img width="1919" height="1025" alt="image" src="https://github.com/user-attachments/assets/7e0246c8-919c-47d4-8a78-2a45e3caedb2" />

## Installation

### Prerequisites
- **Qt 6.6+** (Core, Widgets, Gui, Network, WebEngineWidgets)
- **CMake 3.20+**
- **Ninja** build system
- **C++20** capable compiler (GCC 12+ / Clang 14+)

### Build from source

```bash
# Clone
git clone https://github.com/qfdevel/Frint-Browser.git
cd Frint

# Build with Makefile
make

# Or with CMake directly
mkdir build && cd build
cmake .. -G Ninja
ninja

# Run
./build/bin/frint_browser
```
### Downloading with npm

```bash
sudo npm install -g @qfdevel/frint-browser
frint-browser
```

### Install system-wide

```bash
sudo make install
```

## Usage

### First Run
The setup wizard will guide you through:
1. Language selection (11 languages)
2. Theme selection
3. Profile creation
4. Privacy tutorial

### Privacy Menu
Access quick privacy toggles from the **Privacy** menu:
- Toggle tracker blocker
- Toggle HTTPS-only
- Toggle GPC
- Toggle fingerprint defense
- Toggle anti-fingerprinting JS
- Toggle WebRTC blocking
- Toggle ETag stripping

### Settings Dialog
Press the hamburger menu (☰) → **Settings** to access all options:
- **General** — Home page, search engine, clear-on-exit
- **Privacy** — Core privacy controls
- **🔐 Advanced** — Individual fingerprinting toggles (Canvas, WebGL, Audio, Font, HW concurrency, etc.)
- **📊 Dashboard** — Privacy score, session statistics, clear data, export report
- **Appearance** — Theme selection, font size, custom CSS
- **Profiles** — Profile management, danger zone
- **🔄 Updates** — Download location, update checking
- **📋 Extensions** — Enable/disable extensions
- **ℹ️ About** — Version information, GitHub link

### Keyboard Shortcuts
| Shortcut | Action |
|----------|--------|
| `Ctrl+T` | New tab |
| `Ctrl+W` | Close tab |
| `Ctrl+R` | Reload |
| `Ctrl+L` | Focus address bar |
| `Ctrl+Q` | Quit |
| `Ctrl+=` / `Ctrl+-` | Zoom in/out |
| `Ctrl+0` | Reset zoom |
| `Alt+Left` / `Alt+Right` | Back / Forward |
| `Alt+Home` | Home page |
| `F6` | Focus address bar |
| `F11` | Full screen |
| `F12` | DevTools |

## Project Structure

```
Frint/
├── CMakeLists.txt        # CMake build configuration
├── Makefile              # Convenience build wrapper
├── resources.qrc         # Qt resource bundle
├── src/
│   ├── main.cpp          # Entry point, first-run wizard
│   ├── browser_window.*  # Main window, toolbar, menus, animations
│   ├── web_view.*        # WebEngine view, privacy integration
│   ├── settings_dialog.* # Settings UI (9 tabs)
│   ├── settings_manager.*# Persistent settings (QSettings)
│   ├── setup_wizard.*    # First-run wizard
│   ├── privacy/
│   │   ├── url_cleaner.*           # Tracking URL parameter stripping
│   │   ├── tracking_blocker.*      # Known tracker domain blocking
│   │   ├── gpc_header.*            # GPC/DNT headers
│   │   ├── referrer_policy.*       # Strict referrer policy
│   │   ├── dns_resolver.*          # DNS-over-HTTPS resolver
│   │   ├── fingerprinting_defender.*# User-agent spoofing
│   │   └── privacy_url_interceptor.*# Request-level interceptor
│   └── storage/
│       └── storage_partition.*     # Storage isolation
├── resources/
│   ├── privacy/
│   │   ├── anti_fingerprint.js     # Anti-fingerprinting script
│   │   ├── cosmetic_blocker.js     # Ad-blocking CSS injection
│   │   ├── tracking_domains.txt    # 25K tracking domains list
│   │   ├── tracking_params.txt     # 41K tracking parameters list
│   │   └── user_agents.txt         # User-agent spoofing list
│   ├── fonts/                      # Bundled fonts (Inter, JetBrains Mono)
│   ├── icons/                      # Application icons
│   ├── help/                       # Onboarding pages
│   └── samples/                    # Demo HTML pages
├── store/
│   ├── themes/            # Theme CSS files
│   ├── extensions/        # Bundled extensions
│   └── store.json         # Extension store manifest
├── configs/
│   └── default_prefs.json # Default settings
└── logo/
    └── logo.png           # Application logo
```

## Privacy Test Results

Frint aims for maximum scores on all major privacy testing tools:

- **Cover Your Tracks** (EFF) — Full protection against all tracking categories
- **Ad Blocker Test** (turtlecute.org) — Blocks ads, analytics, social trackers, error trackers, OEM trackers, and more
- **DNS Leak Test** — All DNS queries through encrypted DoH

### Trackers Blocked
- Google Ads/DoubleClick ecosystem
- Facebook/Meta pixels
- Twitter/X analytics
- Amazon ad systems
- Microsoft/Bing ads
- Adobe Audience Manager
- AppNexus/Xandr
- Criteo, Taboola, Outbrain
- LinkedIn, TikTok, Pinterest, Reddit, Snapchat
- Yahoo, Yandex, Unity Ads
- Media.net, AdColony
- Google Analytics, Hotjar, MouseFlow, LuckyOrange, FreshWorks
- BugSnag, Sentry, Rollbar, Datadog
- All major error trackers and analytics platforms

## License

GNU General Public License v3.0 — see [LICENSE](LICENSE) for details.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Support

- GitHub Issues: [github.com/qfdevel/Frint-Browser/issues](https://github.com/qfdevel/Frint-Browser/issues)
- GitHub Repository: [github.com/qfdevel/Frint-Browser](https://github.com/qfdevel/Frint-Browser)

---

*Built with Qt 6.8.2 and the Frint Privacy Engine*
