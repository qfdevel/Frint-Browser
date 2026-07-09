# Changelog

## [0.1.0] - 2026-07-09

### ✨ Major Features

#### 🎨 Complete Rendering Engine
- HTML parsing using libxml2 with full DOM tree construction
- Complete CSS engine: parser, selector matching, cascade, computed styles
- Full layout engine: block, inline, flexbox, and grid formatting contexts
- QPainter-based rendering with anti-aliasing and smooth text
- CSS box model with margin, border, padding, and content areas
- CSS animations and transitions (60fps)
- CSS transforms (translate, rotate, scale, skew)
- Box shadows, text shadows, and border-radius
- Background colors and images
- Form element rendering (input, button, textarea, select, checkbox, radio)

#### 🔒 Privacy Protection (All Enabled by Default)
- **UrlCleaner**: Strips 200+ tracking parameters (fbclid, gclid, utm_\*, etc.)
- **ReferrerPolicy**: Strict-origin-when-cross-origin policy enforcement
- **TrackingBlocker**: Blocks 150+ known ad/tracking domains
- **FingerprintingDefender**: Spoofs fonts, canvas, screen, timezone, language, concurrency, memory, user agent
- **DnsResolver**: DNS-over-HTTPS via Cloudflare with response caching
- **GpcHeader**: Sends `Sec-GPC: 1` on all HTTP requests
- **StoragePartition**: SHA256 origin-hashed isolation for all storage

#### 🧩 Complete Browser UI
- Tabbed interface with multiple pages
- Address bar with DuckDuckGo search fallback
- Navigation buttons (back, forward, reload, stop, home)
- Privacy menu with toggleable controls (tracker blocker, HTTPS-only, GPC, fingerprinting)
- Status bar showing privacy status
- Keyboard shortcuts for common actions
- New tab, close tab, reorder tabs
- History stack with back/forward navigation

#### ⚡ JavaScript Engine
- Duktape-based JavaScript execution
- Web API bindings: window, document, console, location, history, navigator
- DOM manipulation: getElementById, querySelector, createElement
- Timer functions: setTimeout, setInterval, clearTimeout, clearInterval
- requestAnimationFrame and cancelAnimationFrame
- Alert, confirm, prompt dialogs
- localStorage and sessionStorage API
- JSON.parse and JSON.stringify
- ES6 polyfills: Promise, String/Array includes, Object.assign
- Console API with log, warn, error, info, debug methods

#### 🌐 Network Layer
- HTTP/HTTPS request handling via QNetworkAccessManager
- Privacy header injection (GPC, DNT, Referrer, User-Agent)
- Cookie management with domain/path matching
- Response caching
- Resource loading for images and external assets

### 🏗️ Architecture
- Modular rendering pipeline: HTML → DOM → CSS → Layout → Paint
- Separation of concerns: DOM, CSS, Layout, Rendering, Network modules
- Privacy layer integrated at every level
- Qt6-based UI with QMainWindow, QTabWidget, QToolBar

### 📦 Build System
- CMake 3.20+ with Ninja
- Qt6 Core, Widgets, Gui, Network, Multimedia
- libxml2 and Duktape external dependencies
- Security hardening flags: _FORTIFY_SOURCE=2, -fstack-protector-strong, -fPIE
- Output directory: `build/bin/frint_browser`

### 🎨 Store & Themes
- Store manifest with 3 themes (Catppuccin Mocha, Nord Dark, Gruvbox Material)
- 3 sample extensions (Dark Reader, uBlock Lite, Privacy Badger)
- Python validation script for security checking
- GitHub Actions CI for automated validation
- Extension JS linting and dangerous pattern detection

### 📚 Documentation
- README with build instructions, features, keyboard shortcuts
- CONTRIBUTING with code style, commit format, PR process
- CHANGELOG with complete feature listing
- API documentation in header files

## [0.0.1] - Initial Project Setup
- Initial CMakeLists.txt and project structure
- Basic Qt6 application with empty window
- Git repository initialization
