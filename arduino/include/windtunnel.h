#ifndef WINDTUNNEL_H
#define WINDTUNNEL_H
#include <stdbool.h>
#include <stdint.h>

#define WT_PERIOD_MS 10u
#define WT_TIMEOUT_MS 1000u
#define WT_SENSOR_MAX_AGE_MS 100u
#define WT_MAX_PERCENT 30.0f /* Voorlopige softwarelimiet, geen motorkalibratie. */

typedef enum { WT_STOPPED, WT_MANUAL, WT_FAULT } WtState;
typedef enum { WT_OK, WT_NOT_READY, WT_INTERLOCK, WT_SENSOR, WT_TIMEOUT,
               WT_TIMING, WT_OUTPUT } WtFault;
typedef enum { WT_CMD_STOP, WT_CMD_RESET, WT_CMD_MANUAL } WtCommandType;
typedef struct {
    uint32_t sampled_ms;
    float air_before_mps, air_after_mps, lift_n, drag_n, fan_rpm;
    bool valid, rpm_valid, door_closed, estop_ok;
} WtSample;
typedef struct { uint32_t id; WtCommandType type; float percent; } WtCommand;
typedef struct {
    WtState state;
    WtFault fault;
    float output_percent;
    uint32_t last_command_ms, last_step_ms;
    bool stepped;
} WtControl;

void wt_init(WtControl *c);
void wt_trip(WtControl *c, WtFault fault);
void wt_step(WtControl *c, const WtSample *s, uint32_t now_ms, bool ready);
bool wt_command(WtControl *c, const WtSample *s, const WtCommand *cmd,
                uint32_t now_ms, bool ready);
#endif
