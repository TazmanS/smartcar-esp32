#include "services/wifi_service.h"

#include <string.h>

#include "app/app_events.h"
#include "app_config.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

static const char *TAG = "wifi_service";

static QueueHandle_t s_app_event_queue;
static bool s_wifi_connected;

static void wifi_service_post_event(app_event_type_t type)
{
	if (s_app_event_queue == NULL) {
		return;
	}

	app_event_t event = {
		.type = type,
	};

	if (xQueueSend(s_app_event_queue, &event, 0) != pdPASS) {
		ESP_LOGW(TAG, "Application queue is full, dropping Wi-Fi event %d", type);
	}
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	(void)arg;
	(void)event_data;

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
		return;
	}

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		s_wifi_connected = false;
		ESP_LOGI(TAG, "Wi-Fi disconnected, retrying connection");
		wifi_service_post_event(APP_EVENT_WIFI_DISCONNECTED);
		esp_wifi_connect();
		return;
	}

	if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
		s_wifi_connected = true;
		ESP_LOGI(TAG, "Wi-Fi connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
		wifi_service_post_event(APP_EVENT_WIFI_CONNECTED);
	}
}

esp_err_t wifi_service_init(QueueHandle_t app_event_queue)
{
	static bool initialized;

	if (initialized) {
		return ESP_OK;
	}

	s_app_event_queue = app_event_queue;

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t wifi_handler_instance;
	esp_event_handler_instance_t ip_handler_instance;

	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		WIFI_EVENT,
		ESP_EVENT_ANY_ID,
		&wifi_event_handler,
		NULL,
		&wifi_handler_instance));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		IP_EVENT,
		IP_EVENT_STA_GOT_IP,
		&wifi_event_handler,
		NULL,
		&ip_handler_instance));

	wifi_config_t wifi_config = {
		.sta = {
			.threshold.authmode = WIFI_AUTH_WPA2_PSK,
			.pmf_cfg = {
				.capable = true,
				.required = false,
			},
		},
	};

	strncpy((char *)wifi_config.sta.ssid, APP_WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
	strncpy((char *)wifi_config.sta.password, APP_WIFI_PASSWORD, sizeof(wifi_config.sta.password) - 1);

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	initialized = true;
	ESP_LOGI(TAG, "Wi-Fi service started");
	return ESP_OK;
}

bool wifi_service_is_connected(void)
{
	return s_wifi_connected;
}