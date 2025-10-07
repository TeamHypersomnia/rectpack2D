cmake_minimum_required(VERSION 3.20.0)

project(rectpack2D)

add_subdirectory(src)
add_library(rectpack2D::rectpack2D ALIAS rectpack2D)

option(RECTPACK2D_BUILD_EXAMPLE "Build example for rectpack2D" OFF)

if(RECTPACK2D_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()