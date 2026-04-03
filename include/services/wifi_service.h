#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include <stdbool.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

esp_err_t wifi_service_init(QueueHandle_t app_event_queue);
bool wifi_service_is_connected(void);

#endif