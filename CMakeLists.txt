cmake_minimum_required(VERSION 3.22)


project(drm_test C)

set(CMAKE_C_STANDARD 11)

set( CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
  ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
  ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

find_package(PkgConfig REQUIRED)
pkg_check_modules(libdrm QUIET libdrm)

if( libdrm_FOUND EQUAL 1 )
    add_definitions( -DLIBDRM )
endif()

add_subdirectory(src)
