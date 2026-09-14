#include <Arduino.h>
extern "C" {
#include "wt_rtos.h"
#include "application_tasks.h"
#include "board_port.h"
}

static void startup_failed()
{
    board_emergency_off();
    noInterrupts();
    for (;;) { }
}

void setup()
{
    if (!windtunnel_start()) startup_failed();
#if defined(ARDUINO_ARCH_SAM)
    vTaskStartScheduler();
    startup_failed(); // Insufficient resources or unexpected scheduler return.
#endif
    // ESP32 Arduino has already started FreeRTOS before calling setup().
}

void loop()
{
#if defined(ARDUINO_ARCH_ESP32)
    delay(1000); // Yield Arduino's loop task; application runs in its own tasks.
#endif
    // DueFreeRTOS calls loop from the idle hook: never block here on the Due.
}

#if defined(ARDUINO_ARCH_SAM)
// Linker wrappers replace the library's blink-only failure behavior.
extern "C" void __wrap_vApplicationMallocFailedHook(void) { startup_failed(); }
extern "C" void __wrap_vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;
    startup_failed();
}
extern "C" void __wrap_vAssertBlink(void) { startup_failed(); }
#endif
