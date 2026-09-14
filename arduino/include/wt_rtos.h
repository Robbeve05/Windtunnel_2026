#ifndef WT_RTOS_H
#define WT_RTOS_H
/* ESP32 bundles FreeRTOS; Due resolves these headers from DueFreeRTOS. */
#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#else
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#endif
#endif
