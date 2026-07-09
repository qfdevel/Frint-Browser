# Frint Browser

A telemetry-free, fully customizable, privacy-focused web browser based on the LibWeb engine (Ladybird).

## Features

- **HTTPS-Only Mode** - Automatically upgrades HTTP to HTTPS
- **Third-Party Cookie Blocking** - Prevents cross-site tracking
- **Tracking Parameter Removal** - Strips `fbclid`, `gclid`, `utm_*`, and 100+ tracking parameters
- **DNS-over-HTTPS (DoH)** - Secure DNS resolution via Cloudflare
- **Global Privacy Control** - Sends `Sec-GPC: 1` header for opt-out
- **Strict Referrer Policy** - Strict-origin-when-cross-origin enforcement
- **Fingerprinting Protection** - Canvas, font, and screen fingerprinting defenses
- **Tracker Blocker** - Blocks 100+ known tracking domains
- **Storage Partitioning** - Origin-isolated storage sandboxes
- **Clear on Exit** - Optional complete data erasure

## PrivacyTests.org Compliance

Frint Browser is designed to pass all [PrivacyTests.org](https://privacytests.org) tests, including:

- State partitioning
- HTTPS-only upgrades
- Referrer policy enforcement
- Tracking parameter removal
- Global Privacy Control (GPC)
- DNS over HTTPS
- Fingerprinting resistance
- Cross-site tracker blocking
- Cross-session clearing

## Building

### Requirements

- Qt6 (Core, Widgets, Gui, Network)
- CMake 3.20+
- C++20 compiler
- Ninja or Make

### Build Steps

```bash
# Install dependencies (Arch Linux)
sudo pacman -S --needed qt6-base qt6-tools qt6-wayland cmake ninja gcc base-devel

# Clone and build
git clone --recursive https://github.com/yourusername/frint-browser.git
cd frint-browser
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -GNinja
ninja -j$(nproc)
```

### Run

```bash
./bin/frint_browser
```

## License

MIT
