#include <drm-test-output.h>
#include <drm-test-utils.h>
#include <sys/ioctl.h>
#include <stdlib.h>

struct drm_mode_card_res get_resources (int dri_fd) {
  // Where we'll store the card query results.
  // For now, using fixed memory. Might be better to dynamically allocate
  // card requests, e.g. for multiple cards and if we need to keep these requests.
  struct drm_mode_card_res res = {0};
  // Pointers to resource ids. Allocated after card gives us resource counts.
  u_int64_t *res_fb_buf,
    *res_crtc_buf,
    *res_conn_buf,
    *res_enc_buf;

  // The presumption is that we have KMS control over card.

  // This call should get the connector count.
  ioctl(dri_fd, DRM_IOCTL_MODE_GETRESOURCES, &res);
 
  // Allocate device identifier arrays now that we have a count.
  res_fb_buf = (u_int64_t*) malloc(sizeof(u_int64_t)*res.count_fbs);
  res_crtc_buf = (u_int64_t*) malloc(sizeof(u_int64_t)*res.count_crtcs);
  res_conn_buf = (u_int64_t*) malloc(sizeof(u_int64_t)*res.count_connectors);
  res_enc_buf = (u_int64_t*) malloc(sizeof(u_int64_t)*res.count_encoders);

  // Now ask the card to fill in resource IDs.
  res.fb_id_ptr = (u_int64_t) res_fb_buf;
  res.crtc_id_ptr = (u_int64_t) res_crtc_buf;
  res.connector_id_ptr = (u_int64_t) res_conn_buf;
  res.encoder_id_ptr = (u_int64_t) res_enc_buf;
  ioctl(dri_fd, DRM_IOCTL_MODE_GETRESOURCES, &res);
  return res;
}

/*Populates a list with outputs retrieved through DRM ioctls.
* Returns the number of outputs fetched in the process.
*/
int get_outputs (int dri_fd, list_link * list_head) {
  list_link * tail = list_head;

  // Get the card resources.
  struct drm_mode_card_res card_res = get_resources(dri_fd);
  u_int32_t connector_count = card_res.count_connectors;
  u_int64_t * connectors = (u_int64_t*) card_res.connector_id_ptr;
  
  // Bulk allocate for connector/outputs.
  struct drm_mode_get_connector * get_connectors = malloc(sizeof(struct drm_mode_get_connector)*connector_count);
  struct output * output = malloc(sizeof(struct output)*connector_count);

  // Get connector details.
  for ( int i = 0; i < connector_count; i++ ) {
    // Need to follow same structure, because we need modes.
    struct drm_mode_get_connector * conn = &get_connectors[i];
    conn->connector_id = connectors[i];
    // Fill in counts.
    ioctl(dri_fd, DRM_IOCTL_MODE_GETCONNECTOR, conn);


    // Allocate space for connector information.
    // We might not need the connector, depending on the counts.
    // But, it won't hurt to grab the connector info anyways.
    conn->modes_ptr = (u_int64_t) malloc(sizeof(struct drm_mode_modeinfo)*conn->count_modes);
    conn->props_ptr = (u_int64_t) malloc(sizeof(u_int64_t)*conn->count_props);
    conn->prop_values_ptr = (u_int64_t) malloc(sizeof(u_int64_t)*conn->count_props);
    conn->encoders_ptr = (u_int64_t) malloc(sizeof(u_int64_t)*conn->count_encoders);

    ioctl(dri_fd, DRM_IOCTL_MODE_GETCONNECTOR, conn);

    // Save the connector to the output.
    output[i].connector = conn;
    tail->next = &output[i].connector_link;
    tail = tail->next;
  }
  return card_res.count_connectors;
}

/*Adds the useful outputs (has a mode and encoder) to the given list.
* If useful_outputs is NULL, a new list is allocated.
* Returns the pointer to the useful outputs list head.
*/
list_link * filter_useful_outputs (list_link * outputs, list_link * useful_outputs) {
    if ( useful_outputs == NULL )
        useful_outputs = malloc(sizeof(list_link));
    
    // Loop through the output list.
    list_link * tail = outputs;
    list_link * useful_tail = useful_outputs;
    while ( ( tail = tail->next ) != NULL ) {
        struct output * output = link_container_of(tail, output, connector_link);
        // Skip over useless connections.
        if (
            output->connector->count_modes < 1 ||
            output->connector->count_encoders < 1 ||
            !output->connector->encoder_id ||
            !output->connector->connection
        ) continue;
        useful_tail->next = &output->useful_link;
        useful_tail = useful_tail->next;
    }

    return useful_outputs;
}