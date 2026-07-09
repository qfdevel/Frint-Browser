cmake_minimum_required(VERSION 3.20)
project(FrintBrowser VERSION 0.1.0 LANGUAGES CXX)

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
add_compile_options(-D_FORTIFY_SOURCE=2)
add_compile_options(-fstack-protector-strong -fPIE -Wformat -Wformat-security)
add_link_options(-pie -Wl,-z,relro -Wl,-z,now)

# ── Find Qt6 ──────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)

# ── Output directory ──────────────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ── Collect all Frint source files ────────────────────────────────────────
file(GLOB_RECURSE FRINT_SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# ── Main executable ───────────────────────────────────────────────────────
add_executable(frint_browser ${FRINT_SOURCES})

target_include_directories(frint_browser PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/configs"
)

target_link_libraries(frint_browser PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
)

# ── Optional: Ladybird LibWeb integration ────────────────────────────────
option(FRINT_USE_LIBWEB "Link against Ladybird LibWeb libraries" OFF)

if(FRINT_USE_LIBWEB)
    message(STATUS "Ladybird LibWeb integration enabled")

    set(LADYBIRD_DIR "${CMAKE_SOURCE_DIR}/ladybird")

    # Include paths for Ladybird headers
    target_include_directories(frint_browser PRIVATE
        "${LADYBIRD_DIR}"
        "${LADYBIRD_DIR}/AK"
        "${LADYBIRD_DIR}/Libraries"
    )

    # Determine Ladybird build output
    if(EXISTS "${LADYBIRD_DIR}/Build")
        set(LADYBIRD_BUILD_DIR "${LADYBIRD_DIR}/Build")
    elseif(EXISTS "${LADYBIRD_DIR}/build")
        set(LADYBIRD_BUILD_DIR "${LADYBIRD_DIR}/build")
    else()
        message(FATAL_ERROR "Ladybird build directory not found.
        Build Ladybird first: cd ladybird && mkdir Build && cd Build && cmake .. -GNinja ...")
    endif()

    target_include_directories(frint_browser PRIVATE
        "${LADYBIRD_BUILD_DIR}"
        "${LADYBIRD_BUILD_DIR}/Libraries/LibWeb"
        "${LADYBIRD_BUILD_DIR}/Libraries/LibJS"
        "${LADYBIRD_BUILD_DIR}/Libraries/LibGfx"
        "${LADYBIRD_BUILD_DIR}/Libraries/LibCore"
        "${LADYBIRD_BUILD_DIR}/Libraries/LibURL"
    )

    # Find Ladybird libraries
    file(GLOB LADYBIRD_LIBS
        "${LADYBIRD_BUILD_DIR}/Libraries/*/liblagom-*.a"
        "${LADYBIRD_BUILD_DIR}/Libraries/*/liblagom-*.so"
    )

    if(LADYBIRD_LIBS)
        target_link_libraries(frint_browser PRIVATE ${LADYBIRD_LIBS})
        target_compile_definitions(frint_browser PRIVATE FRINT_HAVE_LIBWEB=1)
        message(STATUS "Found ${LADYBIRD_LIBS} Ladybird libraries")
    else()
        message(WARNING "Ladybird libraries not built yet. Building without LibWeb.")
    endif()
else()
    message(STATUS "Ladybird LibWeb integration disabled. Building with Qt-only renderer.")
endif()

# ── Copy configs to build directory ──────────────────────────────────────
add_custom_command(TARGET frint_browser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/configs"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../configs"
    COMMENT "Copying configuration files"
)

# ── Install rules ─────────────────────────────────────────────────────────
install(TARGETS frint_browser RUNTIME DESTINATION bin)
install(DIRECTORY configs/ DESTINATION share/frint-browser/configs)
install(FILES README.md DESTINATION share/doc/frint-browser)
