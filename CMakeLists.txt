cmake_minimum_required(VERSION 3.20)
project(FrintBrowser VERSION 1.0.0 LANGUAGES CXX)

# ── C++ Standard ──────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Qt Auto MOC / RCC / UIC ───────────────────────────────────────────────
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Security hardening ────────────────────────────────────────────────────
add_compile_definitions(FRINT_SECURE_DEFAULTS)
add_compile_options(-O2)
add_compile_options(-D_FORTIFY_SOURCE=2)
add_compile_options(-fstack-protector-strong -fPIE -Wformat -Wformat-security)
add_link_options(-pie -Wl,-z,relro -Wl,-z,now)

# ── Find Qt6 ──────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network WebEngineWidgets)

# ── Output directory ──────────────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ── Collect all Frint source files ────────────────────────────────────────
file(GLOB_RECURSE FRINT_SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# ── Qt Resource file ──────────────────────────────────────────────────────
set(CMAKE_AUTORCC ON)
set(RESOURCE_FILE "${CMAKE_SOURCE_DIR}/resources.qrc")

# ── Main executable ───────────────────────────────────────────────────────
add_executable(frint_browser ${FRINT_SOURCES} ${RESOURCE_FILE})

target_include_directories(frint_browser PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/configs"
)

target_link_libraries(frint_browser PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
    Qt6::WebEngineWidgets
)
