#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#include <stdbool.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

esp_err_t mqtt_service_init(QueueHandle_t app_event_queue);
esp_err_t mqtt_service_publish_status(const char *payload);
bool mqtt_service_is_connected(void);

#endif