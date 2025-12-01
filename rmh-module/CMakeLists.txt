cmake_minimum_required(VERSION 3.15)
project(RMH_Module VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(ENABLE_GPU "Enable GPU acceleration" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Find dependencies
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# TensorFlow Lite (adjust path as needed)
set(TFLITE_INCLUDE_DIR "/usr/local/include/tensorflow/lite" CACHE PATH "TensorFlow Lite include directory")
set(TFLITE_LIB_DIR "/usr/local/lib" CACHE PATH "TensorFlow Lite library directory")

# OpenCV (optional, for image processing)
find_package(OpenCV QUIET)
if(OpenCV_FOUND)
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
    add_definitions(-DHAVE_OPENCV)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/signals
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/autonomy
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${OPENSSL_INCLUDE_DIR}
    ${TFLITE_INCLUDE_DIR}
)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -march=native)
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        add_compile_options(-g -O0 -fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()
endif()

# Signal Processing Library
add_library(rmh_signals
    signals/signal_processor.cpp
    signals/sonar_processor.cpp
    signals/mag_processor.cpp
    signals/ml_classifier.cpp
    signals/sensor_fusion.cpp
)
target_link_libraries(rmh_signals
    PUBLIC
        ${OpenSSL_LIBRARIES}
        Threads::Threads
)

if(OpenCV_FOUND)
    target_link_libraries(rmh_signals PUBLIC ${OpenCV_LIBS})
endif()

# Crypto Communications Library
add_library(rmh_crypto
    crypto/crypto_comms.cpp
    crypto/encryption_engine.cpp
    crypto/key_exchange.cpp
    crypto/spread_spectrum.cpp
    crypto/time_sync.cpp
)
target_link_libraries(rmh_crypto
    PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
)

# Autonomy Controller Library
add_library(rmh_autonomy
    autonomy/autonomy_controller.cpp
    autonomy/path_planner.cpp
    autonomy/navigation_controller.cpp
    autonomy/pid_controller.cpp
    autonomy/obstacle_avoidance.cpp
    autonomy/lost_comms_recovery.cpp
)
target_link_libraries(rmh_autonomy
    PUBLIC
        Threads::Threads
        m  # Math library
)

# Main RMH Module (combines all components)
add_library(rmh_module
    rmh_module.cpp
)
target_link_libraries(rmh_module
    PUBLIC
        rmh_signals
        rmh_crypto
        rmh_autonomy
)

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS rmh_signals rmh_crypto rmh_autonomy rmh_module
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY signals/ crypto/ autonomy/
    DESTINATION include/rmh
    FILES_MATCHING PATTERN "*.h"
)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/RMHModuleConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/RMHModuleConfigVersion.cmake"
    DESTINATION lib/cmake/RMHModule
)

# Print build summary
message(STATUS "")
message(STATUS "RMH Module Build Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  GPU acceleration: ${ENABLE_GPU}")
message(STATUS "  OpenSSL version: ${OPENSSL_VERSION}")
if(OpenCV_FOUND)
    message(STATUS "  OpenCV version: ${OpenCV_VERSION}")
endif()
message(STATUS "")
