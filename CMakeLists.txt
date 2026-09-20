cmake_minimum_required(VERSION 3.24)

project(LanCue LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt tooling needs these on for every target that touches Qt.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Warnings-as-errors is picked per-compiler here so every subdirectory inherits
# the same flags without re-deciding this per target. See §4/Phase 0 in the roadmap:
# CI must fail on compiler warnings for new code from day one.
if(WIN32)
    include(windows-toolchain)
elseif(APPLE)
    include(macos-toolchain)
elseif(UNIX)
    include(linux-toolchain)
endif()

include(qt-deploy-platform-plugin)

enable_testing()

add_subdirectory(core-lib)
add_subdirectory(lancue-core)
add_subdirectory(lancue-settings)
add_subdirectory(lancue-cli)
add_subdirectory(tools)
add_subdirectory(tests)
