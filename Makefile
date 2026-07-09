# ── Frint Browser Makefile ───────────────────────────────────────────────
# Simple wrapper around CMake build system.
# Usage:
#   make          – build (default)
#   make debug    – build with debug flags
#   make clean    – remove build artifacts
#   make rebuild  – clean + build
#   make run      – build and run
#   make install  – install to /usr/local (requires sudo)

BUILD_DIR ?= build
BINARY    := $(BUILD_DIR)/bin/frint_browser
CMAKE     := cmake
NINJA     := ninja

.PHONY: all build debug clean rebuild run install

all: build

# ── Configure + build ──
$(BUILD_DIR)/build.ninja:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) .. -G Ninja

build: $(BUILD_DIR)/build.ninja
	cd $(BUILD_DIR) && $(NINJA)

# ── Debug build ──
debug: $(BUILD_DIR)/build.ninja
	cd $(BUILD_DIR) && $(CMAKE) .. -G Ninja -DCMAKE_BUILD_TYPE=Debug && $(NINJA)

# ── Clean ──
clean:
	rm -rf $(BUILD_DIR)

# ── Rebuild from scratch ──
rebuild: clean build

# ── Run ──
run: build
	./$(BINARY)

# ── Install ──
install: build
	install -Dm755 $(BINARY) /usr/local/bin/frint_browser
	install -Dm644 resources.qrc /usr/local/share/frint/resources.qrc
	@echo "Installed frint_browser to /usr/local/bin/"
