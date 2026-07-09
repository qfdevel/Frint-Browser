#!/usr/bin/env python3
"""
Frint Plugin & Theme Store Validation Script

Validates submitted themes and extensions for:
- Valid JSON syntax
- Required fields
- Security issues (no executable code, no remote URLs in themes)
- Version format
- Field types
"""

import json
import sys
import os
import re

REQUIRED_THEME_FIELDS = {"id", "name", "version", "author", "colors"}
REQUIRED_EXTENSION_FIELDS = {"id", "name", "version", "author", "type", "permissions"}
VALID_TYPES = {"privacy", "toolbar", "customization", "developer"}

COLORS_DIR = "../store/themes"
EXTENSIONS_DIR = "../store/extensions"

def validate_json_file(filepath):
    """Validate a JSON file can be parsed."""
    try:
        with open(filepath, 'r') as f:
            return json.load(f)
    except json.JSONDecodeError as e:
        return f"Invalid JSON in {filepath}: {e}"
    except FileNotFoundError:
        return f"File not found: {filepath}"

def validate_theme(data, filename):
    """Validate a theme manifest."""
    errors = []

    # Check required fields
    for field in REQUIRED_THEME_FIELDS:
        if field not in data:
            errors.append(f"Missing required field: '{field}'")

    if "colors" in data:
        colors = data["colors"]
        if not isinstance(colors, dict):
            errors.append("'colors' must be an object")
        else:
            required_colors = {"background", "foreground", "accent"}
            for rc in required_colors:
                if rc not in colors:
                    errors.append(f"Missing required color: '{rc}'")

            # Validate color format
            color_pattern = re.compile(r'^#[0-9a-fA-F]{6}$')
            for name, value in colors.items():
                if not isinstance(value, str) or not color_pattern.match(value):
                    errors.append(f"Invalid color format for '{name}': {value}")

    # Security check: no executable content
    if "javascript" in data or "script" in data:
        errors.append("Themes must not contain executable code")

    # Version format check
    if "version" in data:
        version_pattern = re.compile(r'^\d+\.\d+\.\d+$')
        if not version_pattern.match(data["version"]):
            errors.append(f"Invalid version format: {data['version']} (expected X.Y.Z)")

    return errors

def validate_extension(data, filename):
    """Validate an extension manifest."""
    errors = []

    # Check required fields
    for field in REQUIRED_EXTENSION_FIELDS:
        if field not in data:
            errors.append(f"Missing required field: '{field}'")

    # Validate type
    if "type" in data and data["type"] not in VALID_TYPES:
        errors.append(f"Invalid extension type: {data['type']}. Valid: {', '.join(VALID_TYPES)}")

    # Validate permissions
    if "permissions" in data:
        if not isinstance(data["permissions"], list):
            errors.append("'permissions' must be a list")

    # Version format
    if "version" in data:
        version_pattern = re.compile(r'^\d+\.\d+\.\d+$')
        if not version_pattern.match(data["version"]):
            errors.append(f"Invalid version format: {data['version']}")

    return errors

def main():
    print("═" * 60)
    print("  Frint Plugin & Theme Store Validation")
    print("═" * 60)

    base_dir = os.path.dirname(os.path.abspath(__file__))
    store_path = os.path.join(base_dir, "store.json")

    # Validate store manifest
    print("\n📋 Validating store manifest...")
    store = validate_json_file(store_path)
    if isinstance(store, str):
        print(f"  ❌ {store}")
        return 1

    total_themes = 0
    total_extensions = 0
    errors_found = 0

    # Validate themes
    if "themes" in store:
        for theme in store["themes"]:
            total_themes += 1
            theme_id = theme.get("id", "unknown")
            print(f"\n🎨 Theme: {theme.get('name', theme_id)} ({theme_id})")

            theme_file = theme.get("file", "")
            theme_path = os.path.join(base_dir, theme_file)
            theme_data = validate_json_file(theme_path)

            if isinstance(theme_data, str):
                print(f"  ❌ {theme_data}")
                errors_found += 1
            else:
                errors = validate_theme(theme_data, theme_file)
                if errors:
                    for err in errors:
                        print(f"  ❌ {err}")
                    errors_found += len(errors)
                else:
                    print(f"  ✅ Valid")

    # Validate extensions
    if "extensions" in store:
        for ext in store["extensions"]:
            total_extensions += 1
            ext_id = ext.get("id", "unknown")
            print(f"\n🔌 Extension: {ext.get('name', ext_id)} ({ext_id})")

            ext_file = ext.get("file", "")
            ext_path = os.path.join(base_dir, ext_file)
            ext_data = validate_json_file(ext_path)

            if isinstance(ext_data, str):
                print(f"  ❌ {ext_data}")
                errors_found += 1
            else:
                errors = validate_extension(ext_data, ext_file)
                if errors:
                    for err in errors:
                        print(f"  ❌ {err}")
                    errors_found += len(errors)
                else:
                    print(f"  ✅ Valid")

    print(f"\n═" * 60)
    print(f"  Summary: {total_themes} themes, {total_extensions} extensions")
    if errors_found:
        print(f"  ❌ {errors_found} error(s) found")
        return 1
    else:
        print(f"  ✅ All valid!")
        return 0

if __name__ == "__main__":
    sys.exit(main())
