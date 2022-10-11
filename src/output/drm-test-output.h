#ifndef __DRM_TEST_OUTPUT_H
#define __DRM_TEST_OUTPUT_H

#include <sys/types.h>
#include <libdrm/drm.h>
#include <cairo.h>
#include <drm-test-utils.h>

struct output {
  // The connector details filled in by ioctl call.
  struct drm_mode_get_connector * connector;

  struct drm_mode_get_encoder encoder;

  void * fb; // The connector associated with the connector.
  long fb_w;
  long fb_h;
  unsigned long long fb_size;

  u_int32_t pitch;

  cairo_surface_t * surface;
  cairo_t * cr;

  struct list_link connector_link;
  struct list_link useful_link;
};


struct drm_mode_card_res get_resources (int dri_fd);

int get_outputs (int dri_fd, struct drm_mode_card_res card_res, list_link * list_head);

list_link * filter_useful_outputs (list_link * outputs, list_link * useful_outputs);

void free_card_resources (struct drm_mode_card_res card_res);

#endif