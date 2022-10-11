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

int main (int argc, char ** argv) {
  int dri_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
  
  struct list_link connectors = {0};

  struct list_link active_connectors = {0};


  // Acquire control of the direct rendering infra device.
  ioctl(dri_fd, DRM_IOCTL_SET_MASTER, 0);

  struct drm_mode_card_res card_res = get_resources(dri_fd);

  int connection_count = get_outputs(dri_fd, card_res, &connectors);

  filter_useful_outputs(&connectors, &active_connectors);

  struct list_link * tail;

  // Start handling active connections.
  // Reuse the tail ptr defined above.
  tail = active_connectors.next;
  while ( tail != NULL ) {
    struct output * connector = link_container_of(tail, connector, useful_link);


    // Using first mode. TODO: Allow later configuartion of modes.
    struct drm_mode_modeinfo * mode = (struct drm_mode_modeinfo*) connector->connector->modes_ptr;
    struct drm_mode_create_dumb create_dumb = {0};
    struct drm_mode_map_dumb map_dumb = {0};
    struct drm_mode_fb_cmd cmd_dumb={0};

    // Framebuffer dimensions.
    create_dumb.width = mode->hdisplay;
    create_dumb.height = mode->vdisplay;

    printf("M1 %d %d\n", create_dumb.width, create_dumb.height);

    // I think bits per pixel. Might be wrong.
    create_dumb.bpp = 32;

    create_dumb.flags = 0;
    create_dumb.pitch = 0;
    create_dumb.size = 0;
    create_dumb.handle = 0;

    // Create the dumb frame buffer. Will give us a GrExMa handle.
    ioctl(dri_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb);

    cmd_dumb.width = create_dumb.width;
    cmd_dumb.height = create_dumb.height;
    cmd_dumb.bpp = create_dumb.bpp;
    cmd_dumb.pitch = create_dumb.pitch;
    cmd_dumb.depth = 24;
    cmd_dumb.handle = create_dumb.handle;

    // Add the "dumb" frame buffer. 
    ioctl(dri_fd, DRM_IOCTL_MODE_ADDFB, &cmd_dumb);

    printf("M1 %d\n", cmd_dumb.pitch);

    // Map the frame buffer.
    map_dumb.handle = create_dumb.handle;
    ioctl(dri_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb);
    printf("M1: %llu\n", map_dumb.offset);
    connector->fb = mmap(0, create_dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED, dri_fd, map_dumb.offset);

    connector->fb_h = create_dumb.height;
    connector->fb_w = create_dumb.width;

    // Get buffer encoder.
    connector->encoder.encoder_id = connector->connector->encoder_id;
    ioctl(dri_fd, DRM_IOCTL_MODE_GETENCODER, &connector->encoder);

    printf("M4: %d\n", connector->encoder.encoder_type);

    struct drm_mode_crtc crtc = {0};
    crtc.crtc_id = connector->encoder.crtc_id;

    printf("M4: %d\n", connector->encoder.crtc_id);
    ioctl(dri_fd, DRM_IOCTL_MODE_GETCRTC, &crtc);

    crtc.fb_id = cmd_dumb.fb_id;
    crtc.set_connectors_ptr = (u_int64_t) &connector->connector->connector_id;
    crtc.count_connectors = 1;
    crtc.mode = *mode;
    crtc.mode_valid = 1;
    ioctl(dri_fd, DRM_IOCTL_MODE_SETCRTC, &crtc);

    connector->pitch = create_dumb.pitch;
    connector->fb_size = create_dumb.size;

    tail = tail->next;
  }

  // Release KMS control.
  ioctl(dri_fd, DRM_IOCTL_DROP_MASTER, 0);

  // Start drawing to the frame buffer.

  tail = active_connectors.next;
  while ( tail != NULL ) {
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
    sleep(5);
    tail = tail->next;
  }

  // Clean up.

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
    tail = tail->next;
    cairo_destroy(connector->cr);
    cairo_surface_destroy(connector->surface);
    munmap(connector->fb, connector->fb_size);
    free(connector);
  }

  return 0;
}

