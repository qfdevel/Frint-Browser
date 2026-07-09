# Frint Browser

**Privacy-first. Telemetry-free. Yours.**

Frint Browser is a fully customizable, privacy-focused web browser built from the ground up to protect your digital footprint. Based on the [Ladybird](https://ladybird.org) LibWeb engine, Frint blocks trackers, strips tracking parameters, enforces HTTPS, and gives you complete control over your browsing data.

## ✨ Features

### 🔒 Privacy (All Enabled by Default)

| Feature | Status |
|---------|--------|
| HTTPS-Only Mode | ✅ Automatic HTTP→HTTPS upgrades |
| Third-Party Cookie Blocking | ✅ Prevents cross-site tracking |
| Tracking Parameter Removal | ✅ Strips 200+ tracking params (fbclid, gclid, utm_*, etc.) |
| DNS-over-HTTPS (DoH) | ✅ Cloudflare, Quad9, or Google endpoints |
| Global Privacy Control (GPC) | ✅ Sends `Sec-GPC: 1` with every request |
| Strict Referrer Policy | ✅ Strict-origin-when-cross-origin enforced |
| Fingerprinting Defense | ✅ Canvas, font, screen, timezone, UA spoofing |
| Tracker Blocker | ✅ Blocks 150+ known tracking domains |
| Storage Partitioning | ✅ Origin-hashed, SHA256-isolated storage |
| Clear on Exit | ✅ Optional complete data erasure |
| Do Not Track | ✅ Legacy DNT: 1 header sent |

### 🧩 Architecture

```
┌─────────────────────────────────────────────┐
│                 Frint Browser                │
├─────────────────────────────────────────────┤
│  BrowserWindow (Tabs, Toolbar, Status bar)  │
├─────────────────────────────────────────────┤
│  WebView (Qt6 network + placeholder render) │
├──────────────────┬──────────────────────────┤
│  Privacy Modules │    Storage               │
│  ┌──────────────┐│  ┌────────────────────┐  │
│  │ UrlCleaner   ││  │ StoragePartition   │  │
│  │ ReferrerPol. ││  │ (Origin-hashed)    │  │
│  │ TrackingBlk  ││  └────────────────────┘  │
│  │ Fingerprint  ││                          │
│  │ DnsResolver  ││  Configs                 │
│  │ GpcHeader    ││  ┌────────────────────┐  │
│  └──────────────┘│  │ default_prefs.json │  │
├──────────────────┴──┴──────────────────────┤
│  Ladybird LibWeb (optional, via -DFRINT_USE_LIBWEB=ON) │
└─────────────────────────────────────────────┘
```

### 🌐 Plugin & Theme Store

A community-driven marketplace for extensions and themes is included at `store/`. See [Frint-Plugin-Theme-Store](store/README.md).

## 📋 PrivacyTests.org Compliance

Frint Browser is designed to pass all [PrivacyTests.org](https://privacytests.org) test categories:

- **State Partitioning** — All storage isolated by `(scheme, host, port)` tuple
- **Navigation Tests** — Strict-origin-when-cross-origin referrer policy, window.name cleared
- **HTTPS Tests** — HTTP→HTTPS upgrades, warnings on insecure
- **Misc Tests** — GPC header sent, DoH enabled by default
- **Fingerprinting Resistance** — Canvas, font, screen, timezone, UA, concurrency, memory spoofed
- **Tracking Query Parameters** — 200+ parameters stripped
- **Tracker Content Blocking** — 150+ domains blocked
- **Tracking Cookie Protection** — Third-party cookies blocked, cookie partitioning
- **Cross-session Tracking** — Clear on exit optional
- **DNS Privacy** — DoH with Cloudflare default

## 🏗️ Build Instructions

### Prerequisites

- **Compiler:** GCC 13+ or Clang 16+ (C++20 required)
- **CMake:** 3.20+
- **Qt6:** Core, Widgets, Gui, Network
- **Ninja** or Make

### Arch Linux

```bash
# Install dependencies
sudo pacman -S --needed qt6-base qt6-tools qt6-wayland cmake ninja gcc base-devel
```

### Build

```bash
# Clone and build
git clone --recursive https://github.com/frint-browser/frint-browser.git
cd frint-browser
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -GNinja
ninja
```

### Build with Ladybird LibWeb Integration

```bash
# First build Ladybird libraries
cd ladybird
mkdir Build && cd Build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release -DENABLE_GUI_TARGETS=ON -DBUILD_LADYBIRD=OFF -DBUILD_LAGOM=OFF
ninja
cd ../..

# Then build Frint with LibWeb
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -GNinja -DFRINT_USE_LIBWEB=ON
ninja
```

### Run

```bash
./build/bin/frint_browser
# Or with a URL
./build/bin/frint_browser https://example.com
# Help
./build/bin/frint_browser --help
```

## 🚀 Usage

| Feature | How |
|---------|-----|
| Open URL | Type in address bar, press Enter |
| New Tab | Click `+` or Ctrl+T |
| Close Tab | Click `×` on tab or Ctrl+W |
| Back/Forward | ← / → buttons |
| Reload | ↻ button or Ctrl+R |
| Privacy Controls | `Privacy` menu in menu bar |
| Clear Storage | `Privacy → Clear All Storage` |

## 🧪 Testing

### Quick Smoke Test

```bash
cd build
./bin/frint_browser https://example.com
```

### Privacy Verification

Check the browser's console output for privacy module logging:

```bash
./bin/frint_browser 2>&1 | grep "\[Frint\]"
```

## 📁 Project Structure

```
frint-browser/
├── CMakeLists.txt          # Build system (Qt6 + optional Ladybird)
├── src/
│   ├── main.cpp            # Entry point
│   ├── browser_window.*    # Tabbed browser UI
│   ├── web_view.*          # Web rendering widget
│   ├── settings_manager.*  # JSON-backed preferences
│   ├── privacy/
│   │   ├── url_cleaner.*   # Tracking parameter removal
│   │   ├── referrer_policy.* # Referrer header enforcement
│   │   ├── tracking_blocker.* # Domain-based tracker blocking
│   │   ├── fingerprinting_defender.* # Anti-fingerprinting
│   │   ├── dns_resolver.*  # DNS-over-HTTPS
│   │   └── gpc_header.*    # Global Privacy Control
│   └── storage/
│       └── storage_partition.* # Origin-isolated storage
├── configs/
│   └── default_prefs.json  # Default settings
├── store/                  # Plugin & Theme Store
├── ladybird/               # Ladybird submodule
└── resources/              # Icons & assets
```

## 📄 License

GNU General Public License v3.0 (GPL-3.0)

See [LICENSE](LICENSE) for details.

## 🤝 Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on:
- Code style
- Commit messages
- Pull requests
- Testing requirements
- Security reporting

## 🔗 Links

- [Ladybird Browser](https://ladybird.org)
- [PrivacyTests.org](https://privacytests.org)
- [Plugin & Theme Store](store/README.md)

## 🙏 Acknowledgments

- The [Ladybird](https://ladybird.org) team for their amazing browser engine
- [PrivacyTests.org](https://privacytests.org) for comprehensive testing methodology
- [Catppuccin](https://catppuccin.com) for the theme color inspiration
- All open-source contributors who make privacy-respecting software possible
