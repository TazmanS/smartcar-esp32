#include "services/camera_service.h"

#include "esp_log.h"
#include "hal/ledc_types.h"
#include "sdkconfig.h"

static const char *TAG = "camera_service";

esp_err_t camera_service_init(void)
{
	camera_config_t config = {
			.pin_pwdn = 32,
			.pin_reset = -1,
			.pin_xclk = 0,
			.pin_sccb_sda = 26,
			.pin_sccb_scl = 27,
			.pin_d7 = 35,
			.pin_d6 = 34,
			.pin_d5 = 39,
			.pin_d4 = 36,
			.pin_d3 = 21,
			.pin_d2 = 19,
			.pin_d1 = 18,
			.pin_d0 = 5,
			.pin_vsync = 25,
			.pin_href = 23,
			.pin_pclk = 22,
			.xclk_freq_hz = 20000000,
			.ledc_timer = LEDC_TIMER_0,
			.ledc_channel = LEDC_CHANNEL_0,
			.pixel_format = PIXFORMAT_JPEG,
			.frame_size = FRAMESIZE_VGA,
			.jpeg_quality = 12,
			.fb_count = 2,
			.grab_mode = CAMERA_GRAB_LATEST,
			.fb_location = CAMERA_FB_IN_PSRAM,
	};

#if !CONFIG_SPIRAM && !CONFIG_ESP32_SPIRAM_SUPPORT
	config.frame_size = FRAMESIZE_QVGA;
	config.jpeg_quality = 16;
	config.fb_count = 1;
	config.fb_location = CAMERA_FB_IN_DRAM;
#endif

	esp_err_t err = esp_camera_init(&config);
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Camera init failed: %s", esp_err_to_name(err));
		return err;
	}

	sensor_t *sensor = esp_camera_sensor_get();
	if (sensor != NULL)
	{
		sensor->set_vflip(sensor, 1);
		sensor->set_brightness(sensor, 1);
		sensor->set_saturation(sensor, -1);
	}

	ESP_LOGI(TAG, "Camera initialized");
	return ESP_OK;
}

camera_fb_t *camera_service_acquire_frame(void)
{
	return esp_camera_fb_get();
}

void camera_service_release_frame(camera_fb_t *frame)
{
	if (frame != NULL)
	{
		esp_camera_fb_return(frame);
	}
}