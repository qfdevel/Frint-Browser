# Contributing to Frint Browser

First off, thank you for considering contributing! Frint is a community-driven project focused on creating a truly private, independent web browser.

## Code of Conduct

- Be respectful and inclusive
- Focus on what's best for the project and its users
- Help others learn and grow

## Code Style

### C++

- **Standard**: C++20
- **Formatting**: Use 4-space indentation, no tabs
- **Naming**:
  - Classes: PascalCase (`WebView`, `HtmlRenderer`)
  - Methods: camelCase (`loadUrl()`, `parseHtml()`)
  - Variables: camelCase (`m_currentUrl`, `m_scrollY`)
  - Constants: PascalCase with underscore prefix for members
- **Headers**: Use `#ifndef` guards with format `FRINT_MODULE_FILE_H`
- **Includes**: Group in order: project headers, Qt headers, system headers
- **Braces**: Allman style for functions, K&R for control flow
- **Comments**: Use `//` for single-line, `/* */` for documentation blocks
- **Nullptr**: Use `nullptr`, never `NULL` or `0`

### Example
```cpp
#ifndef FRINT_WEB_VIEW_H
#define FRINT_WEB_VIEW_H

#include <QWidget>
#include <QUrl>

namespace Frint {

class WebView : public QWidget {
    Q_OBJECT

public:
    explicit WebView(QWidget *parent = nullptr);
    ~WebView() override;

    void loadUrl(const QUrl &url);
    QUrl currentUrl() const { return m_currentUrl; }

signals:
    void urlChanged(const QUrl &url);

private:
    QUrl m_currentUrl;
};

} // namespace Frint

#endif
```

### JavaScript (for extensions)
- **Standard**: ES6+
- **Formatting**: 2-space indentation
- **Naming**: camelCase for functions/variables, PascalCase for classes
- **Semicolons**: Required

## Commit Format

```
<type>(<scope>): <description>

<optional body>

<optional footer>
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `chore`

Examples:
```
feat(renderer): add flexbox layout support
fix(privacy): correct GPC header format for Cloudflare
docs(README): update build instructions for macOS
```

## Pull Request Process

1. Create a feature branch from `main`
2. Make your changes following the code style
3. Ensure the project builds: `ninja`
4. Test your changes with real web pages
5. Update documentation if needed
6. Submit a PR with a clear description of changes

## Testing Requirements

- All new rendering features should be tested with a sample HTML file
- Privacy module changes should verify no regressions in existing protections
- JavaScript engine changes should not break existing page rendering
- Performance-sensitive changes should include benchmark data

## Project Structure

```
src/
├── main.cpp                 # Application entry point
├── browser_window.h/cpp     # Main browser window (tabs, toolbar, menus)
├── web_view.h/cpp           # Web content area (networking + rendering)
├── settings_manager.h/cpp   # User settings persistence
├── privacy/                 # Privacy protection modules
│   ├── url_cleaner.h/cpp    # Tracking parameter removal
│   ├── referrer_policy.h/cpp # Referrer policy enforcement
│   ├── tracking_blocker.h/cpp # Domain-based tracking protection
│   ├── fingerprinting_defender.h/cpp # Anti-fingerprinting
│   ├── dns_resolver.h/cpp   # DNS-over-HTTPS
│   └── gpc_header.h/cpp     # Global Privacy Control
├── storage/                 # Storage partitioning
│   ├── storage_partition.h/cpp
│   ├── session_storage.h/cpp
│   └── local_storage.h/cpp
└── renderer/                # Rendering engine
    ├── html_renderer.h/cpp  # Pipeline orchestrator
    ├── dom/                 # DOM implementation
    ├── css/                 # CSS engine
    ├── layout/              # Layout engine
    ├── rendering/           # QPainter rendering
    ├── javascript/          # Duktape JS engine
    └── network/             # Network layer
```

## Reporting Issues

- Use GitHub Issues
- Include the URL you were visiting
- Describe expected vs actual behavior
- Include error messages if any
- Mention your OS and Frint version

## Feature Requests

- Check existing issues first
- Describe the use case, not just the solution
- Consider privacy implications

## Security

If you discover a security vulnerability:
- Do NOT open a public issue
- Email the maintainers directly
- Allow 48 hours for response before disclosure

Thank you for helping make Frint better! ❤️
