#ifndef __DRM_TEST_DRAW_H
#define __DRM_TEST_DRAW_H

#include <sys/types.h>
#include <libdrm/drm.h>
#include <cairo.h>
#include <drm-test-utils.h>

struct output;

typedef struct frame_descriptor {
    void * fb;
    long fb_h;
    long fb_w;

    u_int32_t fb_id;
    unsigned long long fb_size; 

    u_int32_t pitch;
} frame_descriptor;

typedef struct draw_context {
    struct output * output;

    // This is redundant, given there will be a frame_descriptor.
    // Equivalent to &output->framebuffer.
    struct frame_descriptor * framebuffer;
    
    cairo_surface_t * surface;
    cairo_t * cr;

} draw_context;

int prepare_buffer ( int dri_fd, struct output * output );

struct draw_context * create_draw_context ( struct output * output );

void destroy_context (struct draw_context * ctx);

#endif