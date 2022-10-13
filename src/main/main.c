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


int main (int argc, char ** argv) {
  // TODO: Argument processing and implement config file(s).
  // Which card to use is device and situation-specific.
  int dri_fd = open("/dev/dri/card0", O_RDWR );
  
  struct list_link connectors = {0};

  struct list_link active_connectors = {0};


  // Acquire control of the direct rendering infra device.
  ioctl(dri_fd, DRM_IOCTL_SET_MASTER, 0);

  struct drm_mode_card_res card_res = get_resources(dri_fd);

  get_outputs(dri_fd, card_res, &connectors);

  filter_useful_outputs(&connectors, &active_connectors);

  struct list_link * tail;
  struct list_link * last_tail;

  // Start handling active connections.
  // Reuse the tail ptr defined above.

  tail = active_connectors.next;
  last_tail = &active_connectors;
  while ( tail != NULL ) {
    struct output * output = link_container_of(tail, output, useful_link);

    char err = 0;

    if ( output_mode_set(output, (struct drm_mode_modeinfo *) output->connector->modes_ptr) ) { 
      perror("Error setting output mode: ");
      err = 1;
    }

    if ( prepare_buffer(dri_fd, output) ) {
      perror("Error creating/mapping dumb buffer: ");
      err = 1;
    }

    if ( apply_buffer(dri_fd, output) ) {
      perror("Error setting CRTC buffer: ");
      err = 1;
    }

    // For now, cut out active connections that
    // throw an error.
    // TODO: Cleanup the faulty connection.
    // Leaving a mess behind as it is.
    if ( err ) last_tail->next = tail->next;

    if ( !err ) last_tail = tail;

    tail = tail->next;
  }

  // Release KMS control.
  ioctl(dri_fd, DRM_IOCTL_DROP_MASTER, 0);

  // Start drawing to the frame buffer.

  tail = &active_connectors;
  while ( ( tail = tail->next ) != NULL ) {
    struct output * output = link_container_of(tail, output, useful_link);

    draw_context * ctx = create_draw_context(output);

    cairo_rectangle (ctx->cr, 100, 100, 200, 200);
    cairo_set_source_rgb (ctx->cr, 100, 0, 100);
    cairo_fill (ctx->cr);
  }
  sleep(5);

  // Clean up.


  if ( ioctl(dri_fd, DRM_IOCTL_SET_MASTER, 0) >= 0 ) {
    tail = &active_connectors;
    while ( ( tail = tail->next ) != NULL ) {
      struct output * connector = link_container_of(tail, connector, useful_link);
      restore_buffer(dri_fd, connector);
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

    destroy_context(connector->ctx);

    munmap(connector->framebuffer.fb, connector->framebuffer.fb_size);

    if (ioctl(dri_fd, DRM_IOCTL_GEM_CLOSE, &gem_close) < 0)
      fprintf(stdout, "WARN: Failed closing GEM handle.\n");

    tail = tail->next;

    free(connector);
  }


  close(dri_fd);

  return 0;
}

