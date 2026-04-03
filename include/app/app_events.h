#ifndef APP_EVENTS_H
#define APP_EVENTS_H

#define APP_MQTT_TOPIC_MAX_LEN 64
#define APP_MQTT_DATA_MAX_LEN 128

typedef enum {
	APP_EVENT_WIFI_CONNECTED = 1,
	APP_EVENT_WIFI_DISCONNECTED,
	APP_EVENT_MQTT_CONNECTED,
	APP_EVENT_MQTT_DISCONNECTED,
	APP_EVENT_MQTT_MESSAGE,
} app_event_type_t;

typedef struct {
	char topic[APP_MQTT_TOPIC_MAX_LEN];
	char data[APP_MQTT_DATA_MAX_LEN];
} app_mqtt_message_t;

typedef struct {
	app_event_type_t type;
	union {
		app_mqtt_message_t mqtt_message;
	} data;
} app_event_t;

#endif