cmake_minimum_required(VERSION 3.28...3.30)

find_package(Qt6 REQUIRED Core Widgets)

if(NOT TARGET OBS::qt-slider-ignorewheel)
  add_subdirectory(
    "${CMAKE_SOURCE_DIR}/shared/qt/slider-ignorewheel"
    "${CMAKE_BINARY_DIR}/shared/qt/slider-ignorewheel"
  )
endif()

add_library(qt-double-slider INTERFACE)
add_library(OBS::qt-double-slider ALIAS qt-double-slider)

target_sources(qt-double-slider INTERFACE double-slider.cpp double-slider.hpp)
target_include_directories(qt-double-slider INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(qt-double-slider INTERFACE OBS::qt-slider-ignorewheel Qt::Core Qt::Widgets)
