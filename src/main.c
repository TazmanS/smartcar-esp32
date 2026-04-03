#include "app/app_controller.h"

#include "esp_check.h"

void app_main(void)
{
  ESP_ERROR_CHECK(app_controller_start());
}