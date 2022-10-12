#include <libdrm/drm.h>
#include <libdrm/drm_mode.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h> // For device control... ioctl
#include <fcntl.h> // File control operations
#include <stddef.h>
#include <sys/mman.h>
#include <cairo.h>
#include <drm-test-utils.h>
#include <drm-test-output.h>
#include <drm-test-draw.h>

int restore_output (int dri_fd, struct output * output) {
    output->original_crtc.set_connectors_ptr = (u_int64_t) &output->connector->connector_id;
    output->original_crtc.count_connectors = 1;
    output->original_crtc.mode_valid = 1;
    if ( ioctl(dri_fd, DRM_IOCTL_MODE_SETCRTC, &output->original_crtc) >= 0)
      return 0;
    return 1;
}

int main (int argc, char ** argv) {
  int dri_fd = open("/dev/dri/card0", O_RDWR );
  
  struct list_link connectors = {0};

  struct list_link active_connectors = {0};


  // Acquire control of the direct rendering infra device.
  ioctl(dri_fd, DRM_IOCTL_SET_MASTER, 0);

  struct drm_mode_card_res card_res = get_resources(dri_fd);

  get_outputs(dri_fd, card_res, &connectors);

  filter_useful_outputs(&connectors, &active_connectors);

  struct list_link * tail;

  // Start handling active connections.
  // Reuse the tail ptr defined above.
  tail = active_connectors.next;
  while ( tail != NULL ) {
    struct output * output = link_container_of(tail, output, useful_link);

    output_mode_set(output, (struct drm_mode_modeinfo *) output->connector->modes_ptr);

    prepare_buffer(dri_fd, output);

    // Get buffer encoder.
    output->encoder.encoder_id = output->connector->encoder_id;
    ioctl(dri_fd, DRM_IOCTL_MODE_GETENCODER, &output->encoder);


    struct drm_mode_crtc crtc = {0};
    crtc.crtc_id = output->encoder.crtc_id;
    
    ioctl(dri_fd, DRM_IOCTL_MODE_GETCRTC, &crtc);

    output->original_crtc = crtc;

    crtc.fb_id = output->fb_id;
    crtc.set_connectors_ptr = (u_int64_t) &output->connector->connector_id;
    crtc.count_connectors = 1;
    crtc.mode = *output->mode;
    crtc.mode_valid = 1;
    ioctl(dri_fd, DRM_IOCTL_MODE_SETCRTC, &crtc);

    tail = tail->next;
  }

  // Release KMS control.
  ioctl(dri_fd, DRM_IOCTL_DROP_MASTER, 0);

  // Start drawing to the frame buffer.

  tail = &active_connectors;
  while ( ( tail = tail->next ) != NULL ) {
    struct output * connector = link_container_of(tail, connector, useful_link);

    connector->surface = cairo_image_surface_create_for_data(
      connector->fb,
      CAIRO_FORMAT_ARGB32,
      connector->fb_w,
      connector->fb_h,
      connector->pitch
    );

    connector->cr = cairo_create(connector->surface);


    cairo_rectangle (connector->cr, 100, 100, 200, 200);
    cairo_set_source_rgb (connector->cr, 100, 0, 100);
    cairo_fill (connector->cr);
  }
  sleep(5);

  // Clean up.


  if ( ioctl(dri_fd, DRM_IOCTL_SET_MASTER, 0) >= 0 ) {
    tail = &active_connectors;
    while ( ( tail = tail->next ) != NULL ) {
      struct output * connector = link_container_of(tail, connector, useful_link);
      restore_output(dri_fd, connector);
    }
    ioctl(dri_fd, DRM_IOCTL_DROP_MASTER, 0);
  }



  free_card_resources(card_res);

  tail = &connectors;
  while ( ( tail = tail->next ) != NULL ) {
    struct output * connector = link_container_of(tail, connector, connector_link);
    free((void*) connector->connector->encoders_ptr);
    free((void*) connector->connector->props_ptr);
    free((void*) connector->connector->prop_values_ptr);
    free((void*) connector->connector->modes_ptr);
  }
  // free(connectors);
  tail = active_connectors.next;
  while ( tail != NULL ) {
    struct output * connector = link_container_of(tail, connector, useful_link);
    struct drm_gem_close gem_close;
    gem_close.handle = connector->handle;

    cairo_destroy(connector->cr);
    cairo_surface_destroy(connector->surface);
    munmap(connector->fb, connector->fb_size);

    if (ioctl(dri_fd, DRM_IOCTL_GEM_CLOSE, &gem_close) < 0)
      fprintf(stdout, "WARN: Failed closing GEM handle.\n");

    tail = tail->next;

    free(connector);
  }


  close(dri_fd);

  return 0;
}

