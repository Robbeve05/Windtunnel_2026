/* Entry point for a Due ASF/FreeRTOS project, not an Arduino IDE sketch. */
#include <asf.h>
#include "application_tasks.h"
#include "board_port.h"
int main(void)
{
    sysclk_init();
    board_init();
    if (windtunnel_start()) vTaskStartScheduler();
    board_emergency_off();
    for (;;) { }
}
void vApplicationIdleHook(void) { }
void vApplicationMallocFailedHook(void)
{
    board_emergency_off();
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}
void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;
    vApplicationMallocFailedHook();
}
