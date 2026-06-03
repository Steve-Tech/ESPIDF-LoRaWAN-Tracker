#pragma once

#include "freertos/task.h"
extern TaskHandle_t gpsTaskHandle;
void gps_task(void);
