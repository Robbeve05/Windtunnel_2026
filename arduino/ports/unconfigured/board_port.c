/* Buildable interface placeholder. Does not access pins or drive a fan. */
#include "board_port.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
void board_init_safe(void) { board_emergency_off(); }
uint32_t board_millis(void) { return (uint32_t)((uint64_t)xTaskGetTickCount() * 1000u / configTICK_RATE_HZ); }
bool board_ready(void) { return false; }
void board_sample(WtSample *s) { memset(s, 0, sizeof(*s)); }
bool board_fan_write(float percent) { return percent == 0; }
void board_emergency_off(void) { }
void board_watchdog_feed(void) { }
bool board_serial_get(char *byte) { (void)byte; return false; }
bool board_serial_write(const char *data, size_t size) { (void)data; (void)size; return false; }
