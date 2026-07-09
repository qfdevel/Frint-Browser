# Frint Plugin & Theme Store

Community-driven marketplace for Frint Browser themes and privacy extensions.

## Structure

```
store/
├── store.json              # Master manifest
├── validate.py             # Validation script
├── README.md               # This file
├── themes/                 # Theme JSON files
│   ├── frint-dark.json
│   ├── frint-light.json
│   └── nord.json
├── extensions/             # Extension JSON files
│   ├── ublock-origin-compat.json
│   ├── noscript-compat.json
│   └── privacy-badger-compat.json
└── .github/workflows/      # CI validation
    └── validate.yml
```

## Submitting a Theme

1. Create a JSON file in `store/themes/` with:

```json
{
    "id": "my-theme",
    "name": "My Theme",
    "version": "1.0.0",
    "author": "Your Name",
    "colors": {
        "background": "#...",
        "foreground": "#...",
        "accent": "#...",
        "success": "#...",
        "warning": "#...",
        "error": "#...",
        "surface0": "#...",
        "surface1": "#...",
        "surface2": "#..."
    }
}
```

2. Add an entry in `store.json` under the `"themes"` array
3. Run `python3 store/validate.py` to verify
4. Submit a pull request

## Submitting an Extension

1. Create a JSON file in `store/extensions/` with:

```json
{
    "id": "my-extension",
    "name": "My Extension",
    "version": "1.0.0",
    "author": "Your Name",
    "description": "What it does",
    "type": "privacy",
    "permissions": ["webRequest", "storage"],
    "file": "extensions/my-extension.json"
}
```

2. Add an entry in `store.json` under the `"extensions"` array
3. Run validation
4. Submit a pull request

## Security

- **Themes** must not contain executable code (no scripts, no JavaScript)
- **Extensions** are sandboxed and require explicit permission declarations
- All submissions are validated by GitHub Actions before merge
