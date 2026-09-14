#ifndef BOARD_PORT_H
#define BOARD_PORT_H
#include "windtunnel.h"
#include <stddef.h>
/* Implement these for Due/ASF or ESP32/ESP-IDF. No Arduino API dependency.
 * Acquisition must return a coherent calibrated SI sample without waiting for
 * a slow sensor. Preserve the acquisition timestamp when returning cached data.
 * All functions called by ControlTask must be bounded well below 10 ms.
 */
void board_init_safe(void);
uint32_t board_millis(void); /* Monotonic milliseconds, wraps at UINT32_MAX. */
bool board_ready(void);     /* True only after drivers/calibration are commissioned. */
void board_sample(WtSample *sample);
bool board_fan_write(float percent); /* 0 must remove drive enable. */
void board_emergency_off(void); /* Must work from startup/failure hooks without RTOS. */
void board_watchdog_feed(void); /* Only fed by a completed control cycle. */
/* Nonblocking RX, bounded TX. Serial task is the only caller. UART 115200 8N1. */
bool board_serial_get(char *byte);
bool board_serial_write(const char *data, size_t size);
#endif
