cmake_minimum_required(VERSION 3.20)
project(PipeSpectrum VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# PipeWire
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)

# SDL3
pkg_check_modules(SDL3 REQUIRED sdl3)

# OpenGL
find_package(OpenGL REQUIRED)

# GLEW
pkg_check_modules(GLEW REQUIRED glew)

# FFTW3 (float version)
pkg_check_modules(FFTW3 REQUIRED fftw3f)

# yaml-cpp
pkg_check_modules(YAMLCPP REQUIRED yaml-cpp)

# Source files
set(SOURCES
    src/main.cpp
    src/AudioCapture.cpp
    src/FFTAnalyzer.cpp
    src/Renderer.cpp
    src/SpectrumMeter.cpp
    src/Config.cpp
)

# Headers
set(HEADERS
    include/AudioCapture.h
    include/FFTAnalyzer.h
    include/Renderer.h
    include/SpectrumMeter.h
    include/Config.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PIPEWIRE_INCLUDE_DIRS}
    ${SDL3_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${FFTW3_INCLUDE_DIRS}
    ${YAMLCPP_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${PIPEWIRE_LIBRARIES}
    ${SDL3_LIBRARIES}
    OpenGL::GL
    ${GLEW_LIBRARIES}
    ${FFTW3_LIBRARIES}
    ${YAMLCPP_LIBRARIES}
    pthread
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra
    -O3 -march=native
    -Wno-unused-parameter
    -Wno-c99-designator
)

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES config.yaml DESTINATION share/${PROJECT_NAME})
