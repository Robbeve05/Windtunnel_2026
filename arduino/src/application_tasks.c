#include "application_tasks.h"
#include "board_port.h"
#include "protocol.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

typedef struct { WtControl control; WtSample sample; } Snapshot;
typedef struct { uint32_t id; bool accepted; WtState state; float percent; } Ack;
static QueueHandle_t commands, acknowledgements, snapshots;
static TaskHandle_t control_handle;

static void control_task(void *arg)
{
    (void)arg;
    Snapshot snap = {0};
    TickType_t wake = xTaskGetTickCount();
    wt_init(&snap.control);
    for (;;) {
        uint32_t now;
        WtCommand command;
        Ack ack = {0};
        bool have_command;
        board_sample(&snap.sample);
        now = board_millis();
        /* Check expired command BEFORE handling newly received commands. */
        wt_step(&snap.control, &snap.sample, now, board_ready());
        have_command = xQueueReceive(commands, &command, 0) == pdTRUE;
        if (have_command) {
            ack.id = command.id;
            ack.accepted = wt_command(&snap.control, &snap.sample, &command, now, board_ready());
        }
        if (!board_fan_write(snap.control.output_percent)) {
            board_emergency_off();
            wt_trip(&snap.control, WT_OUTPUT);
            ack.accepted = false;
        }
        if (have_command) {
            ack.state = snap.control.state;
            ack.percent = snap.control.output_percent;
            xQueueOverwrite(acknowledgements, &ack);
        }
        xQueueOverwrite(snapshots, &snap);
        board_watchdog_feed();
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(WT_PERIOD_MS));
    }
}

static void communication_task(void *arg)
{
    (void)arg;
    char input[WT_LINE_SIZE], output[WT_LINE_SIZE], byte;
    size_t used = 0;
    bool discard = false, pending = false;
    uint32_t last_sample = board_millis();
    for (;;) {
        Ack ack;
        if (pending && xQueueReceive(acknowledgements, &ack, 0) == pdTRUE) {
            int n = snprintf(output, sizeof(output), "WT1,A,%lu,%d,%d,%.3f\n",
                (unsigned long)ack.id, ack.accepted, (int)ack.state, (double)ack.percent);
            if (n > 0 && (size_t)n < sizeof(output)) board_serial_write(output, (size_t)n);
            pending = false;
        }
        /* Byte budget prevents a noisy serial peer from monopolizing this task.
         * One outstanding request; leave subsequent bytes in the RX ring. */
        for (unsigned budget = 0; !pending && budget < 64 && board_serial_get(&byte); ++budget) {
            if (byte == '\n') {
                WtCommand command;
                input[used] = '\0';
                if (!discard && wt_parse_command(input, &command))
                    pending = xQueueSend(commands, &command, 0) == pdTRUE;
                else board_serial_write("WT1,E,BAD_COMMAND\n", sizeof("WT1,E,BAD_COMMAND\n") - 1);
                used = 0;
                discard = false;
            } else if (byte != '\r') {
                if (byte < 32 || byte > 126 || used >= sizeof(input) - 1) discard = true;
                if (!discard) input[used++] = byte;
            }
        }
        if ((uint32_t)(board_millis() - last_sample) >= 100u) {
            Snapshot snap;
            last_sample = board_millis();
            if (xQueuePeek(snapshots, &snap, 0) == pdTRUE) {
                size_t n = wt_format_sample(output, sizeof(output), &snap.control, &snap.sample);
                if (n) board_serial_write(output, n);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

bool windtunnel_start(void)
{
    board_init_safe();
    if (pdMS_TO_TICKS(WT_PERIOD_MS) == 0) return false;
    commands = xQueueCreate(1, sizeof(WtCommand));
    acknowledgements = xQueueCreate(1, sizeof(Ack));
    snapshots = xQueueCreate(1, sizeof(Snapshot));
    if (!commands || !acknowledgements || !snapshots) goto fail;
    /* Conservative initial allocations. ESP-IDF counts stack bytes; vanilla
     * FreeRTOS counts StackType_t entries. Measure high-water marks on target. */
    if (xTaskCreate(control_task, "Control", 2048, NULL, 3, &control_handle) != pdPASS) goto fail;
    if (xTaskCreate(communication_task, "PiLink", 4096, NULL, 1, NULL) != pdPASS) {
        vTaskDelete(control_handle);
        goto fail;
    }
    return true;
fail:
    board_emergency_off();
    if (commands) vQueueDelete(commands);
    if (acknowledgements) vQueueDelete(acknowledgements);
    if (snapshots) vQueueDelete(snapshots);
    return false;
}
