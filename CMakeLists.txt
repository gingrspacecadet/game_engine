cmake_minimum_required(VERSION 3.10)
project(GameEngine)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/protocols)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND REQUIRED wayland-client)

include_directories(${WAYLAND_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${WAYLAND_LIBRARY_DIRS})

# Manually specify Wayland include directories and libraries
find_path(WAYLAND_INCLUDE_DIR NAMES wayland-client.h PATHS /usr/include /usr/local/include)
find_library(WAYLAND_CLIENT_LIB NAMES wayland-client PATHS /usr/lib /usr/local/lib)
find_library(WAYLAND_CURSOR_LIB NAMES wayland-cursor PATHS /usr/lib /usr/local/lib)
find_library(WAYLAND_EGL_LIB NAMES wayland-egl PATHS /usr/lib /usr/local/lib)

if (NOT WAYLAND_INCLUDE_DIR OR NOT WAYLAND_CLIENT_LIB)
    message(FATAL_ERROR "Wayland development files not found. Please install Wayland development packages.")
endif()

include_directories(${WAYLAND_INCLUDE_DIR})
set(WAYLAND_LIBRARIES ${WAYLAND_CLIENT_LIB} ${WAYLAND_CURSOR_LIB} ${WAYLAND_EGL_LIB})

# Add xdg-shell protocol files
set(XDG_SHELL_PROTOCOL_DIR ${CMAKE_SOURCE_DIR}/src/protocols)
set(XDG_SHELL_CLIENT_HEADER ${XDG_SHELL_PROTOCOL_DIR}/xdg-shell-client-protocol.h)
set(XDG_SHELL_CLIENT_CODE ${XDG_SHELL_PROTOCOL_DIR}/xdg-shell-client-protocol.c)

# Include the generated header
include_directories(${XDG_SHELL_PROTOCOL_DIR})

# Add the generated client code to the build
add_library(xdg-shell STATIC ${XDG_SHELL_CLIENT_CODE})

# Find Vulkan
find_package(Vulkan REQUIRED)

# Define the executable target
add_executable(game_engine
    src/main.cpp
    src/engine.cpp
    src/platform/vulkan_context.cpp
)

# Include directories
target_include_directories(game_engine PRIVATE include)

# Link required libraries
target_link_libraries(game_engine PRIVATE
    xdg-shell
    wayland-client
    xkbcommon
    ${Vulkan_LIBRARIES}
)
