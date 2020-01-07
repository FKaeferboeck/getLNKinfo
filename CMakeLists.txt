cmake_minimum_required(VERSION 3.0)

set(PROJECT_NAME getLNKinfo)
project(${PROJECT_NAME})

# The version number.
set(GETLNKINFO_VERSION_MAJOR 1)
set(GETLNKINFO_VERSION_MINOR 0)
set(GETLNKINFO_BUILD_NUMBER 0)

set(SOLUTION_INCLUDE_DIR "${PROJECT_SOURCE_DIR}")
set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}")

if(NOT WIN32 OR NOT MSVC)
  message(FATAL_ERROR "This project is only available for Windows/MSVC because it's for reading Windows link files")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_UNICODE /DUNICODE /MP /DNOMINMAX")
add_definitions("/wd4297") # function assumed not to throw an exception but does

source_group("Resource Files" FILES getLNKinfo.rc)
add_executable(${PROJECT_NAME} getLNKinfo.cpp stuff.cpp getLNKinfo.h resource.h getLNKinfo.rc)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
