#include "net/stream_server.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "services/camera_service.h"

static const char *TAG = "stream_server";

static httpd_handle_t s_httpd_handle;

static esp_err_t index_handler(httpd_req_t *req)
{
	static const char response[] =
		"<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>ESP32-CAM</title></head>"
		"<body><h1>ESP32-CAM Stream</h1><img src=\"/stream\" style=\"width:100%;max-width:960px;\"></body></html>";

	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t stream_handler(httpd_req_t *req)
{
	static const char *stream_content_type = "multipart/x-mixed-replace;boundary=frame";
	static const char *stream_boundary = "\r\n--frame\r\n";
	static const char *stream_part = "Content-Type: image/jpeg\r\nContent-Length: %zu\r\nX-Timestamp: %lld\r\n\r\n";
	char header[128];

	httpd_resp_set_type(req, stream_content_type);
	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

	for (;;) {
		camera_fb_t *frame = camera_service_acquire_frame();
		if (frame == NULL) {
			ESP_LOGW(TAG, "Failed to acquire frame");
			return ESP_FAIL;
		}

		int header_len = snprintf(header, sizeof(header), stream_part, frame->len, (long long)esp_timer_get_time());
		esp_err_t err = httpd_resp_send_chunk(req, stream_boundary, strlen(stream_boundary));
		if (err == ESP_OK) {
			err = httpd_resp_send_chunk(req, header, header_len);
		}
		if (err == ESP_OK) {
			err = httpd_resp_send_chunk(req, (const char *)frame->buf, frame->len);
		}

		camera_service_release_frame(frame);

		if (err != ESP_OK) {
			ESP_LOGI(TAG, "Stream client disconnected");
			return err;
		}
	}
}

esp_err_t stream_server_start(void)
{
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.server_port = APP_STREAM_PORT;
	config.ctrl_port = APP_STREAM_PORT + 1;
	config.max_open_sockets = 2;

	esp_err_t err = httpd_start(&s_httpd_handle, &config);
	if (err != ESP_OK) {
		return err;
	}

	httpd_uri_t index_uri = {
		.uri = "/",
		.method = HTTP_GET,
		.handler = index_handler,
		.user_ctx = NULL,
	};
	httpd_register_uri_handler(s_httpd_handle, &index_uri);

	httpd_uri_t stream_uri = {
		.uri = "/stream",
		.method = HTTP_GET,
		.handler = stream_handler,
		.user_ctx = NULL,
	};
	httpd_register_uri_handler(s_httpd_handle, &stream_uri);

	ESP_LOGI(TAG, "HTTP stream server started on port %d", config.server_port);
	return ESP_OK;
}