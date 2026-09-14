#ifndef WT_APPLICATION_TASKS_H
#define WT_APPLICATION_TASKS_H
#include <stdbool.h>
/* Call once before starting FreeRTOS on Due, or from app_main on ESP32. */
bool windtunnel_start(void);
#endif
