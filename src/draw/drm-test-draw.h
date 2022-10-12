#ifndef __DRM_TEST_DRAW_H
#define __DRM_TEST_DRAW_H

#include <sys/types.h>
#include <libdrm/drm.h>
#include <cairo.h>
#include <drm-test-utils.h>
#include <drm-test-output.h>

int prepare_buffer ( int dri_fd, struct output * output );

#endif