#include <drm-test-draw.h>
#include <drm-test-output.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

/**
* Pass an output for which a buffer should be prepared.
* Sets output.{fb, fb_size, fb_w, fb_h} accordingly.
* @returns 0 if successful, 1 otherwise.
*/
int prepare_buffer ( int dri_fd, struct output * output ) {
  struct drm_mode_create_dumb create_dumb = {0};
  struct drm_mode_map_dumb map_dumb = {0};
  struct drm_mode_fb_cmd cmd_dumb={0};
  // We need a mode set.
  if ( output->mode == NULL ) return 1;

  create_dumb.width = output->mode->hdisplay;
  create_dumb.height = output->mode->vdisplay;

  create_dumb.bpp = 32;

  create_dumb.flags = 0;
  create_dumb.handle = 0;
  create_dumb.pitch = 0;
  create_dumb.size = 0;

  // Create the "dumb" buffer.
  if ( ioctl(dri_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb))
    return 1;
  
  // Prepare to add the dumb buffer.
  cmd_dumb.width = create_dumb.width;
  cmd_dumb.height = create_dumb.height;
  cmd_dumb.bpp = create_dumb.bpp;
  cmd_dumb.pitch = create_dumb.pitch;
  cmd_dumb.depth = 24;
  cmd_dumb.handle = create_dumb.handle;
  if ( ioctl(dri_fd, DRM_IOCTL_MODE_ADDFB, &cmd_dumb))
    return 1;

  // Map the frame buffer. This will get us an offset from GEM.
  map_dumb.handle = create_dumb.handle;
  if ( ioctl(dri_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb))
    return 1;
  
  output->fb = mmap(0, create_dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED, dri_fd, map_dumb.offset);
  output->fb_h = create_dumb.height;
  output->fb_w = create_dumb.width;
  output->fb_size = create_dumb.size;
  output->fb_id = cmd_dumb.fb_id;
  output->pitch = cmd_dumb.pitch;
  output->handle = create_dumb.handle;

  return 1;
}