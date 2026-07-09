# Contributing to Frint Browser

Thank you for your interest in contributing! Frint Browser is a privacy-first project, and every contribution helps make the web safer.

## Code of Conduct

Be respectful, inclusive, and constructive. We don't tolerate harassment, discrimination, or toxicity.

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone --recursive https://github.com/your-username/frint-browser.git`
3. Build the project (see README.md)
4. Create a branch: `git checkout -b my-feature`
5. Make changes
6. Test thoroughly
7. Submit a pull request

## Code Style

### C++

- Use C++20 features where appropriate
- Follow the existing code style (snake_case for functions/variables, PascalCase for classes)
- Use Qt6 conventions (signals/slots, Q_OBJECT macro)
- 4-space indentation, no tabs
- Maximum line length: 100 characters
- Include guards: `#ifndef FRINT_MODULE_NAME_H`
- Add `qDebug()` logging for key operations prefixed with `[Frint ModuleName]`

### Privacy Modules

Each privacy module must:
1. Have a singleton accessor (`static ClassName &instance()`)
2. Be toggleable at runtime (`setEnabled(bool)`)
3. Log its operations to `qDebug` with `[Frint ModuleName]` prefix
4. Be controllable via `SettingsManager`
5. Have configuration keys in `configs/default_prefs.json`

### Commit Messages

```
<type>: <short description>

<optional detailed description>

<optional issue reference>
```

Types:
- `feat:` — New feature
- `fix:` — Bug fix
- `privacy:` — Privacy enhancement
- `docs:` — Documentation
- `refactor:` — Code restructuring
- `build:` — Build system changes
- `store:` — Plugin/theme store changes

Examples:
```
feat: add DNS-over-HTTPS support with Cloudflare endpoint
fix: handle redirect responses correctly in WebView
privacy: expand tracking parameter list to 200+ entries
```

## Pull Request Process

1. Ensure the project builds with zero warnings
2. Test with `./build/bin/frint_browser`
3. Verify privacy modules log correctly: `./build/bin/frint_browser 2>&1 | grep "\[Frint\]"`
4. Update README.md if adding new features
5. Update CHANGELOG.md with your changes
6. Add appropriate labels (privacy, enhancement, bug, etc.)

## Testing Requirements

All contributions must:
- **Build cleanly** — No compilation errors or warnings
- **Run without crashes** — Browser opens, navigation works
- **Maintain privacy** — Existing privacy features must still pass PrivacyTests.org scenarios

For privacy-related changes, provide:
- What PrivacyTests.org test it addresses
- Verification steps with console output

## Security Reporting

**Do not file public issues for security vulnerabilities.**

Send security reports to: security@frintbrowser.dev

We will:
- Acknowledge receipt within 48 hours
- Provide a timeline for fix
- Credit you in the release notes

## Adding to the Plugin/Theme Store

See `store/README.md` for instructions on submitting themes and extensions.

## License

By contributing, you agree that your contributions will be licensed under GPL-3.0.
