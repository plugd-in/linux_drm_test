cmake_minimum_required(VERSION 3.22)

project(drm_test C)

set(CMAKE_C_STANDARD 11)

set( CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET cairo )

add_executable(drm_test main.c)

target_link_Libraries(drm_test
    PRIVATE drm-test-utils drm-test-output drm-test-draw PkgConfig::deps
)

# target_compile_options(drm PRIVATE -DWLR_USE_UNSTABLE)