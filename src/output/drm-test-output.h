#ifndef __DRM_TEST_OUTPUT_H
#define __DRM_TEST_OUTPUT_H

#include <libdrm/drm_mode.h>
#include <sys/types.h>
#include <libdrm/drm.h>
#include <cairo.h>
#include <drm-test-utils.h>
#include <drm-test-draw.h>

struct output {
  // The connector details filled in by ioctl call.
  struct drm_mode_get_connector * connector;

  struct drm_mode_get_encoder encoder;

  struct drm_mode_modeinfo * mode; // Useful to keep around and reference.

  struct frame_descriptor framebuffer;
  
  u_int32_t handle; // GEM handle.

  // Will probably move to draw heading/structures later.
  cairo_surface_t * surface;
  cairo_t * cr;

  struct list_link connector_link;
  struct list_link useful_link;

  struct drm_mode_crtc original_crtc;
};


struct drm_mode_card_res get_resources (int dri_fd);

int get_outputs (int dri_fd, struct drm_mode_card_res card_res, list_link * list_head);

list_link * filter_useful_outputs (list_link * outputs, list_link * useful_outputs);

void free_card_resources (struct drm_mode_card_res card_res);

int output_mode_set (struct output * output, struct drm_mode_modeinfo * mode);

int restore_buffer (int dri_fd, struct output * output);

int apply_buffer (int dri_fd, struct output * output);

#endif