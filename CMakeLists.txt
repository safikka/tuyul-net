cmake_minimum_required(VERSION 3.14)
project(TuyulEngine VERSION 1.0 LANGUAGES CXX)

# Enforce Modern C++17 Standard global definition
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Code Coverage option specification (Triggers only under Debug build profiles)
option(ENABLE_COVERAGE "Enable localized Code Coverage analysis" OFF)
if(ENABLE_COVERAGE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "[TUYUL-BUILD] Code Coverage tracking enabled for Debug profile.")
    add_compile_options(--coverage -O0 -fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
endif()

# Global trigger to activate CMake's native automated testing framework layout
enable_testing()

# Define local vendored GoogleTest source pathway
set(GTEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tuyul-lib/googletest")

if(EXISTS "${GTEST_DIR}/CMakeLists.txt")
    message(STATUS "[TUYUL-BUILD] Located offline GoogleTest vendor package at: ${GTEST_DIR}")
    
    # Disable default standalone installation targets to keep build layout pristine
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    
    # Incorporate localized GoogleTest source tree into compilation sequence
    add_subdirectory(${GTEST_DIR} googletest_build)
else()
    message(WARNING "[TUYUL-BUILD] Directory ${GTEST_DIR} is missing or empty! Offline unit tests cannot build.")
endif()

# Register modular sub-component project directories
add_subdirectory(tuyul-tcp)
add_subdirectory(tuyul-udp)
add_subdirectory(tuyul-example)
