#include "app/app_controller.h"

#include <string.h>

#include "app/app_events.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "net/stream_server.h"
#include "nvs_flash.h"
#include "services/camera_service.h"
#include "services/mqtt_service.h"
#include "services/wifi_service.h"

static const char *TAG = "app_controller";

static QueueHandle_t s_app_event_queue;

static void app_handle_mqtt_command(const char *topic, const char *payload)
{
	if (topic == NULL || payload == NULL)
	{
		return;
	}

	ESP_LOGI(TAG, "MQTT rx topic=%s payload=%s", topic, payload);

	if (strcmp(payload, "stop") == 0)
	{
		ESP_LOGI(TAG, "Received stop command");
		return;
	}

	if (strcmp(payload, "left") == 0)
	{
		ESP_LOGI(TAG, "Received left command");
		return;
	}

	if (strcmp(payload, "right") == 0)
	{
		ESP_LOGI(TAG, "Received right command");
		return;
	}

	if (strcmp(payload, "forward") == 0)
	{
		ESP_LOGI(TAG, "Received forward command");
		return;
	}

	ESP_LOGW(TAG, "Unsupported MQTT command: %s", payload);
}

static void app_event_task(void *pv_parameters)
{
	(void)pv_parameters;

	app_event_t event;

	for (;;)
	{
		if (xQueueReceive(s_app_event_queue, &event, portMAX_DELAY) != pdPASS)
		{
			continue;
		}

		switch (event.type)
		{
		case APP_EVENT_WIFI_CONNECTED:
			ESP_LOGI(TAG, "Wi-Fi connected");
			ESP_ERROR_CHECK(mqtt_service_init(s_app_event_queue));
			break;

		case APP_EVENT_WIFI_DISCONNECTED:
			ESP_LOGW(TAG, "Wi-Fi disconnected");
			break;

		case APP_EVENT_MQTT_CONNECTED:
			ESP_LOGI(TAG, "MQTT connected");
			break;

		case APP_EVENT_MQTT_DISCONNECTED:
			ESP_LOGW(TAG, "MQTT disconnected");
			break;

		case APP_EVENT_MQTT_MESSAGE:
			app_handle_mqtt_command(
					event.data.mqtt_message.topic,
					event.data.mqtt_message.data);
			break;

		default:
			break;
		}
	}
}

esp_err_t app_controller_start(void)
{
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

	s_app_event_queue = xQueueCreate(10, sizeof(app_event_t));
	if (s_app_event_queue == NULL)
	{
		return ESP_ERR_NO_MEM;
	}

	if (xTaskCreate(app_event_task, "app_event_task", 4096, NULL, 5, NULL) != pdPASS)
	{
		return ESP_FAIL;
	}

	ESP_ERROR_CHECK(camera_service_init());
	ESP_ERROR_CHECK(wifi_service_init(s_app_event_queue));
	ESP_ERROR_CHECK(stream_server_start());

	ESP_LOGI(TAG, "Application started");
	return ESP_OK;
}