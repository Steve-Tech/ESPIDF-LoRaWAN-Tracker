#include <freertos/FreeRTOS.h>
#include <esp_log.h>

#include "tasks/lorawan.h"
#include "tasks/gps.h"

static const char* TAG = "main";

TaskHandle_t lorawanTaskHandle;
TaskHandle_t gpsTaskHandle;

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI(TAG, "Hello world!");
    xTaskCreate((TaskFunction_t)&lorawan_task, "lorawan_task", 4096, NULL, 2, &lorawanTaskHandle);
    xTaskCreate((TaskFunction_t)&gps_task, "gps_task", 4096, NULL, 1, &gpsTaskHandle);

    // Delete this task
    vTaskDelete(NULL);
    vTaskDelay(portMAX_DELAY);
}
