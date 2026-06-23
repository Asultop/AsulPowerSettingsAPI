cmake_minimum_required(VERSION 3.14)
project(AsulPowerSettingsApi VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_CLI   "Build CLI application" ON)

# SDK library
add_subdirectory(src/sdk)

# CLI application
if(BUILD_CLI)
    add_subdirectory(src/cli)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
