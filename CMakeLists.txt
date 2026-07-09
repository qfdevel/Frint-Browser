cmake_minimum_required(VERSION 3.20)
project(FrintBrowser VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Security hardening
add_compile_definitions(FRINT_SECURE_DEFAULTS)
add_compile_options(-D_FORTIFY_SOURCE=2)
add_compile_options(-fstack-protector-strong -fPIE)
add_link_options(-pie)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)

# Collect all source files
file(GLOB_RECURSE FRINT_SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# Create the executable
add_executable(frint_browser ${FRINT_SOURCES})

# Link Qt6
target_link_libraries(frint_browser PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
)

# Include directories
target_include_directories(frint_browser PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/configs"
)

# Copy configs to build directory
add_custom_command(TARGET frint_browser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/configs"
        "$<TARGET_FILE_DIR:frint_browser>/../configs"
    COMMENT "Copying configuration files"
)

# Copy resources to build directory
add_custom_command(TARGET frint_browser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:frint_browser>/../resources"
    COMMENT "Copying resource files"
)

install(TARGETS frint_browser RUNTIME DESTINATION bin)
install(DIRECTORY configs/ DESTINATION share/frint-browser/configs)
install(FILES README.md DESTINATION share/doc/frint-browser)
