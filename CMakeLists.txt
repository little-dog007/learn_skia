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

