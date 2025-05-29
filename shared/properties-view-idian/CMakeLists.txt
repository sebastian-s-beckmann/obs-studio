cmake_minimum_required(VERSION 3.28...3.30)

find_package(Qt6 REQUIRED Core Widgets)

if(NOT TARGET OBS::idian)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/qt/idian" "${CMAKE_BINARY_DIR}/shared/qt/idian")
endif()

if(NOT TARGET OBS::qt-wrappers)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/qt/wrappers" "${CMAKE_BINARY_DIR}/shared/qt/wrappers")
endif()

if(NOT TARGET OBS::qt-double-slider)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/qt/double-slider" "${CMAKE_BINARY_DIR}/shared/qt/double-slider")
endif()

if(NOT TARGET OBS::qt-slider-ignorewheel)
  add_subdirectory(
    "${CMAKE_SOURCE_DIR}/shared/qt/slider-ignorewheel"
    "${CMAKE_BINARY_DIR}/shared/qt/slider-ignorewheel"
  )
endif()

if(NOT TARGET OBS::qt-vertical-scroll-area)
  add_subdirectory(
    "${CMAKE_SOURCE_DIR}/shared/qt/vertical-scroll-area"
    "${CMAKE_BINARY_DIR}/shared/qt/vertical-scroll-area"
  )
endif()

add_library(properties-view-idian STATIC)
add_library(OBS::properties-view-idian ALIAS properties-view-idian)

target_sources(
  properties-view-idian
  PUBLIC PropertiesView.hpp
  PRIVATE PropertiesView.cpp PropertiesView_Property.cpp PropertiesView_Property_List.cpp
)

set_target_properties(properties-view-idian PROPERTIES AUTOMOC ON FOLDER "frontend")

target_include_directories(properties-view-idian PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(
  properties-view-idian
  PUBLIC
    OBS::libobs
    OBS::idian
    OBS::qt-double-slider
    OBS::qt-slider-ignorewheel
    OBS::qt-vertical-scroll-area
    OBS::qt-wrappers
    Qt::Core
    Qt::Widgets
)
