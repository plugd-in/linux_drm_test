add_library(drm-test-draw "")

target_sources(
    drm-test-draw
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/drm-test-draw.c
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/drm-test-draw.h
)


find_package(PkgConfig REQUIRED)
pkg_check_modules(drm-test-draw REQUIRED IMPORTED_TARGET cairo )

target_include_directories(
    drm-test-draw
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}
)

 target_link_Libraries(
    drm-test-draw drm-test-output drm-test-utils PkgConfig::drm-test-draw
)

# target_compile_options(drm-test-draw PUBLIC -DWLR_USE_UNSTABLE)