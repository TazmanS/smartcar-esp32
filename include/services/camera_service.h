#ifndef CAMERA_SERVICE_H
#define CAMERA_SERVICE_H

#include "esp_camera.h"
#include "esp_err.h"

esp_err_t camera_service_init(void);
camera_fb_t *camera_service_acquire_frame(void);
void camera_service_release_frame(camera_fb_t *frame);

#endif