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
#include <cairo/cairo.h>


#define link_container_of(ptr, sample, member) \
	(__typeof__(sample))((char *)(ptr) - \
			     offsetof(__typeof__(*sample), member))


struct list_link;

struct list_link {
  struct list_link * next;
};

struct connector_resource {
  // The connector details filled in by ioctl call.
  struct drm_mode_get_connector * connector;

  struct drm_mode_get_encoder encoder;

  void * fb; // The connector associated with the connector.
  long fb_w;
  long fb_h;

  u_int32_t pitch;

  cairo_surface_t * surface;
  cairo_t * cr;

  u_int64_t connector_id;

  struct list_link link;
};



int main (int argc, char ** argv) {
  int dri_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
  
  // Render/device resource information.
  // Better to be safe with dynamic allocation.
  u_int64_t *res_fb_buf,
    *res_crtc_buf,
    *res_conn_buf,
    *res_enc_buf;

  struct drm_mode_card_res res = {0};

  struct drm_mode_get_connector * connectors;

  struct list_link active_connectors = {0};


  // Acquire control of the direct rendering infra device.
  ioctl(dri_fd, DRM_IOCTL_SET_MASTER, 0);


  // This call should get the connector count.
  ioctl(dri_fd, DRM_IOCTL_MODE_GETRESOURCES, &res);
 
  // Allocate device identifier arrays now that we have a count.
  res_fb_buf = (u_int64_t*) malloc(sizeof(u_int64_t)*res.count_fbs);
  res_crtc_buf = (u_int64_t*) malloc(sizeof(u_int64_t)*res.count_crtcs);
  res_conn_buf = (u_int64_t*) malloc(sizeof(u_int64_t)*res.count_connectors);
  res_enc_buf = (u_int64_t*) malloc(sizeof(u_int64_t)*res.count_encoders);

  // We need some space for later connector queries.
  connectors = (struct drm_mode_get_connector *) malloc(sizeof(struct drm_mode_get_connector)*res.count_connectors);

  // Add the arrays to card res struct.
  res.fb_id_ptr = (u_int64_t) res_fb_buf;
  res.crtc_id_ptr = (u_int64_t) res_crtc_buf;
  res.connector_id_ptr = (u_int64_t) res_conn_buf;
  res.encoder_id_ptr = (u_int64_t) res_enc_buf;
  // This second call will get resource IDs.
  ioctl(dri_fd, DRM_IOCTL_MODE_GETRESOURCES, &res);

  struct list_link * tail = &active_connectors;

  // Get card information (HDMI, VGA, etc.).
  // Loop through connectors. Remember useful connectives (active_conn...)
  for ( int i = 0; i < res.count_connectors; i++ ) {
    // Need to follow same structure, because we need modes.
    struct drm_mode_get_connector * connector = &connectors[i];
    connector->connector_id = res_conn_buf[i];
    // Fill in counts.
    ioctl(dri_fd, DRM_IOCTL_MODE_GETCONNECTOR, connector);


    // Allocate space for connector information.
    // We might not need the connector, depending on the counts.
    // But, it won't hurt to grab the connector info anyways.
    connector->modes_ptr = (u_int64_t) malloc(sizeof(struct drm_mode_modeinfo)*connector->count_modes);
    connector->props_ptr = (u_int64_t) malloc(sizeof(u_int64_t)*connector->count_props);
    connector->prop_values_ptr = (u_int64_t) malloc(sizeof(u_int64_t)*connector->count_props);
    connector->encoders_ptr = (u_int64_t) malloc(sizeof(u_int64_t)*connector->count_encoders);

    ioctl(dri_fd, DRM_IOCTL_MODE_GETCONNECTOR, connector);

    // Skip active connection adding if inactive.
    if (
      connector->count_encoders < 1 ||
      connector->count_modes < 1 ||
      !connector->encoder_id ||
      !connector->connection
    ) continue;

    // Fill in the active connector info.
    struct connector_resource * conn_resource
      = (struct connector_resource *) malloc(sizeof(struct connector_resource));


    conn_resource->connector = connector;
    conn_resource->connector_id = res_conn_buf[i];
    tail->next = &conn_resource->link;
    tail = &conn_resource->link;
  }



  // Start handling active connections.
  // Reuse the tail ptr defined above.
  tail = active_connectors.next;
  while ( tail != NULL ) {
    struct connector_resource * connector = link_container_of(tail, connector, link);


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

    tail = tail->next;
  }

  // Release KMS control.
  ioctl(dri_fd, DRM_IOCTL_DROP_MASTER, 0);

  // Start drawing to the frame buffer.

  tail = active_connectors.next;
  while ( tail != NULL ) {
    struct connector_resource * connector = link_container_of(tail, connector, link);

    connector->surface = cairo_image_surface_create_for_data(
      connector->fb,
      CAIRO_FORMAT_ARGB32,
      connector->fb_w,
      connector->fb_h,
      connector->pitch
    );

    connector->cr = cairo_create(connector->surface);


    cairo_rectangle (connector->cr, 0, 0, 200, 200);
    cairo_set_source_rgb (connector->cr, 100, 100, 100);
    cairo_fill (connector->cr);
    sleep(5);
    tail = tail->next;
  }

  // Clean up.


  free(res_fb_buf);
  free(res_crtc_buf);
  free(res_conn_buf);
  free(res_enc_buf);
  for ( int i = 0; i < res.count_connectors; i++ ) {
    free((void*) connectors[i].encoders_ptr);
    free((void*) connectors[i].props_ptr);
    free((void*) connectors[i].prop_values_ptr);
    free((void*) connectors[i].modes_ptr);
  }
  free(connectors);
  tail = active_connectors.next;
  while ( tail != NULL ) {
    struct connector_resource * connector = link_container_of(tail, connector, link);
    tail = tail->next;
    cairo_destroy(connector->cr);
    cairo_surface_destroy(connector->fb);
    free(connector);
  }

  return 0;
}

