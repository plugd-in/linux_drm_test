add_library(drm-test-output "")

target_sources(
    drm-test-output
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/drm-test-output.c
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/drm-test-output.h
)


find_package(PkgConfig REQUIRED)
pkg_check_modules(drm-test-output REQUIRED IMPORTED_TARGET cairo )

target_include_directories(
    drm-test-output
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}
)

 target_link_Libraries(
    drm-test-output drm-test-utils drm-test-draw PkgConfig::drm-test-output
)

# target_compile_options(drm-test-output PUBLIC -DWLR_USE_UNSTABLE)