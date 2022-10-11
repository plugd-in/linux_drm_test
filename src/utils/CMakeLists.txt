add_library(drm-test-utils "")

target_sources(
    drm-test-utils
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/drm-test-utils.c
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/drm-test-utils.h
)


# find_package(PkgConfig REQUIRED)
# pkg_check_modules(drm-test-util REQUIRED IMPORTED_TARGET )

target_include_directories(
    drm-test-utils
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}
)

 target_link_Libraries(
    drm-test-utils 
)

# target_compile_options(drm-test-utils PUBLIC -DWLR_USE_UNSTABLE)