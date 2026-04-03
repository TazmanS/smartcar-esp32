#include "services/mqtt_service.h"

#include <string.h>

#include "app/app_events.h"
#include "app_config.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "mqtt_service";

static QueueHandle_t s_app_event_queue;
static esp_mqtt_client_handle_t s_client;
static bool s_connected;

static void mqtt_service_post_simple_event(app_event_type_t type)
{
	if (s_app_event_queue == NULL) {
		return;
	}

	app_event_t event = {
		.type = type,
	};

	if (xQueueSend(s_app_event_queue, &event, 0) != pdPASS) {
		ESP_LOGW(TAG, "Application queue is full, dropping MQTT event %d", type);
	}
}

static void mqtt_service_post_message_event(const esp_mqtt_event_handle_t event)
{
	if (s_app_event_queue == NULL) {
		return;
	}

	app_event_t app_event = {
		.type = APP_EVENT_MQTT_MESSAGE,
	};

	size_t topic_len = event->topic_len < (APP_MQTT_TOPIC_MAX_LEN - 1) ? (size_t)event->topic_len : (APP_MQTT_TOPIC_MAX_LEN - 1);
	size_t data_len = event->data_len < (APP_MQTT_DATA_MAX_LEN - 1) ? (size_t)event->data_len : (APP_MQTT_DATA_MAX_LEN - 1);

	memcpy(app_event.data.mqtt_message.topic, event->topic, topic_len);
	app_event.data.mqtt_message.topic[topic_len] = '\0';

	memcpy(app_event.data.mqtt_message.data, event->data, data_len);
	app_event.data.mqtt_message.data[data_len] = '\0';

	if (xQueueSend(s_app_event_queue, &app_event, 0) != pdPASS) {
		ESP_LOGW(TAG, "Application queue is full, dropping MQTT payload");
	}
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	(void)handler_args;
	(void)base;

	esp_mqtt_event_handle_t event = event_data;

	switch ((esp_mqtt_event_id_t)event_id) {
	case MQTT_EVENT_CONNECTED:
		s_connected = true;
		ESP_LOGI(TAG, "Connected to broker");
		esp_mqtt_client_subscribe(s_client, APP_MQTT_SUB_TOPIC, 1);
		mqtt_service_post_simple_event(APP_EVENT_MQTT_CONNECTED);
		break;

	case MQTT_EVENT_DISCONNECTED:
		s_connected = false;
		ESP_LOGI(TAG, "Disconnected from broker");
		mqtt_service_post_simple_event(APP_EVENT_MQTT_DISCONNECTED);
		break;

	case MQTT_EVENT_DATA:
		mqtt_service_post_message_event(event);
		break;

	default:
		break;
	}
}

esp_err_t mqtt_service_init(QueueHandle_t app_event_queue)
{
	s_app_event_queue = app_event_queue;

	esp_mqtt_client_config_t mqtt_cfg = {
		.broker.address.uri = APP_MQTT_BROKER_URI,
		.credentials.username = APP_MQTT_USERNAME,
		.credentials.authentication.password = APP_MQTT_PASSWORD,
	};

	s_client = esp_mqtt_client_init(&mqtt_cfg);
	if (s_client == NULL) {
		return ESP_FAIL;
	}

	ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
	ESP_ERROR_CHECK(esp_mqtt_client_start(s_client));

	ESP_LOGI(TAG, "MQTT service started");
	return ESP_OK;
}

esp_err_t mqtt_service_publish_status(const char *payload)
{
	if (!s_connected || s_client == NULL) {
		return ESP_ERR_INVALID_STATE;
	}

	int message_id = esp_mqtt_client_publish(s_client, APP_MQTT_PUB_TOPIC, payload, 0, 1, 0);
	return message_id >= 0 ? ESP_OK : ESP_FAIL;
}

bool mqtt_service_is_connected(void)
{
	return s_connected;
}