#include "application_tasks.h"
#include "board_port.h"
void app_main(void)
{
    /* ESP-IDF already runs the scheduler. The selected ESP32 board_port.c
     * contains commented hardware examples and remains disabled. */
    if (!windtunnel_start()) board_emergency_off();
}
