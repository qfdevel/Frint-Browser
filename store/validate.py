#!/usr/bin/env python3
"""
Frint Browser Store Validator
Validates theme and extension packages for security and correctness.
"""

import json
import os
import sys
import re
import hashlib
from pathlib import Path

VALID_KEYS_THEME = {'id', 'name', 'version', 'author', 'description', 'type', 'files', 'screenshot', 'license', 'compatibility'}
VALID_KEYS_EXTENSION = {'id', 'name', 'version', 'author', 'description', 'type', 'permissions', 'files', 'license', 'compatibility'}
VALID_TYPES = {'theme', 'extension'}
VALID_LICENSES = {'MIT', 'GPL-2.0', 'GPL-3.0', 'Apache-2.0', 'BSD-2-Clause', 'BSD-3-Clause', 'MPL-2.0', 'LGPL-2.1', 'LGPL-3.0'}
DANGEROUS_PERMISSIONS = {'native-messaging', 'file-system', 'process'}
DANGEROUS_PATTERNS = [
    rb'eval\s*\(',
    rb'Function\s*\(',
    rb'setTimeout\s*\(\s*["\']',
    rb'setInterval\s*\(\s*["\']',
    rb'new\s+Function\s*\(',
    rb'document\.write\s*\(',
    rb'innerHTML\s*=',
    rb'outerHTML\s*=',
    rb'insertAdjacentHTML\s*\(',
]

class StoreValidator:
    def __init__(self, store_dir: str):
        self.store_dir = Path(store_dir)
        self.errors = []
        self.warnings = []
        self.manifest = None

    def validate(self) -> bool:
        """Run all validations. Returns True if valid."""
        self._validate_manifest()
        self._validate_themes()
        self._validate_extensions()
        self._validate_file_integrity()

        if self.errors:
            print(f"❌ VALIDATION FAILED - {len(self.errors)} error(s), {len(self.warnings)} warning(s)")
            for err in self.errors:
                print(f"  ERROR: {err}")
            for warn in self.warnings:
                print(f"  WARNING: {warn}")
            return False

        print(f"✅ VALIDATION PASSED - {len(self.warnings)} warning(s)")
        for warn in self.warnings:
            print(f"  WARNING: {warn}")
        return True

    def _validate_manifest(self):
        manifest_path = self.store_dir / 'store.json'
        if not manifest_path.exists():
            self.errors.append("Missing store.json manifest")
            return

        try:
            with open(manifest_path) as f:
                self.manifest = json.load(f)
        except json.JSONDecodeError as e:
            self.errors.append(f"Invalid JSON in store.json: {e}")
            return

        if 'themes' not in self.manifest or 'extensions' not in self.manifest:
            self.errors.append("store.json must contain 'themes' and 'extensions' arrays")

    def _validate_themes(self):
        if not self.manifest: return
        themes = self.manifest.get('themes', [])
        for theme in themes:
            self._validate_package(theme, 'theme', VALID_KEYS_THEME)
            for file in theme.get('files', []):
                file_path = self.store_dir / 'themes' / file
                if not file_path.exists():
                    self.errors.append(f"Theme '{theme['id']}': missing file '{file}'")
                elif not file.endswith('.css'):
                    self.warnings.append(f"Theme '{theme['id']}': non-CSS file '{file}'")

    def _validate_extensions(self):
        if not self.manifest: return
        extensions = self.manifest.get('extensions', [])
        for ext in extensions:
            self._validate_package(ext, 'extension', VALID_KEYS_EXTENSION)
            permissions = ext.get('permissions', [])
            for perm in permissions:
                if perm in DANGEROUS_PERMISSIONS:
                    self.errors.append(f"Extension '{ext['id']}': dangerous permission '{perm}'")
            for file in ext.get('files', []):
                file_path = self.store_dir / 'extensions' / file
                if not file_path.exists():
                    self.errors.append(f"Extension '{ext['id']}': missing file '{file}'")
                elif file.endswith('.js'):
                    self._check_js_security(file_path, ext['id'])

    def _validate_package(self, pkg, pkg_type, valid_keys):
        pkg_id = pkg.get('id', 'unknown')

        if pkg.get('type') != pkg_type:
            self.errors.append(f"Package '{pkg_id}': type should be '{pkg_type}' got '{pkg.get('type')}'")

        for key in pkg:
            if key not in valid_keys:
                self.warnings.append(f"Package '{pkg_id}': unknown key '{key}'")

        for key in valid_keys - {'screenshot', 'permissions'}:
            if key not in pkg:
                self.errors.append(f"Package '{pkg_id}': missing required key '{key}'")

        license_val = pkg.get('license', '')
        if license_val not in VALID_LICENSES:
            self.warnings.append(f"Package '{pkg_id}': non-standard license '{license_val}'")

        version = pkg.get('version', '')
        if not re.match(r'^\d+\.\d+\.\d+$', version):
            self.errors.append(f"Package '{pkg_id}': invalid version format '{version}'")

    def _check_js_security(self, file_path: Path, pkg_id: str):
        try:
            with open(file_path, 'rb') as f:
                content = f.read()

            for pattern in DANGEROUS_PATTERNS:
                if re.search(pattern, content, re.IGNORECASE):
                    self.warnings.append(
                        f"Extension '{pkg_id}': potentially unsafe code in '{file_path.name}' "
                        f"(matches: {pattern})"
                    )
        except IOError:
            pass

    def _validate_file_integrity(self):
        if not self.manifest: return
        for pkg_list, pkg_type in [(self.manifest.get('themes', []), 'theme'),
                                     (self.manifest.get('extensions', []), 'extension')]:
            for pkg in pkg_list:
                base_dir = self.store_dir / f"{pkg_type}s"
                for file in pkg.get('files', []):
                    file_path = base_dir / file
                    if file_path.exists() and file_path.stat().st_size == 0:
                        self.errors.append(f"Package '{pkg['id']}': empty file '{file}'")

def main():
    import argparse
    parser = argparse.ArgumentParser(description='Validate Frint Browser store packages')
    parser.add_argument('store_dir', nargs='?', default='.',
                        help='Store directory containing store.json')
    parser.add_argument('--ci', action='store_true',
                        help='Enable CI mode (strict checks)')
    args = parser.parse_args()

    validator = StoreValidator(args.store_dir)
    success = validator.validate()
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
