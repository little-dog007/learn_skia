cmake_minimum_required(VERSION 3.28)
project(untitled1)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 CONFIG REQUIRED)
find_package(unofficial-skia CONFIG REQUIRED)

add_executable(untitled1 main.cpp)

target_link_libraries(untitled1 PRIVATE unofficial::skia::skia)
target_link_libraries(untitled1 PRIVATE glfw)

find_package(sdl2 CONFIG REQUIRED)

add_executable(raster raster/main.cpp)
target_link_libraries(raster PRIVATE unofficial::skia::skia SDL2::SDL2)

add_executable(metal
        metal/main_mac.mm
        metal/main_mac.mm
        metal/Window_mac.mm
        metal/GaneshMetalWindowContext_mac.mm
        metal/MetalWindowContext.mm
        util/WindowContext.cpp
        util/Windows.cpp
        util/Application.h
        util/editor_application.cpp)
target_include_directories(metal PRIVATE util)

target_link_libraries(metal PRIVATE unofficial::skia::skia SDL2::SDL2)


