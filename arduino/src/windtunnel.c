#include "windtunnel.h"
#include <math.h>
#include <string.h>

static WtFault check(const WtSample *s, uint32_t now, bool ready)
{
    if (!ready) return WT_NOT_READY;
    if (!s->door_closed || !s->estop_ok) return WT_INTERLOCK;
    if (!s->valid || (uint32_t)(now - s->sampled_ms) > WT_SENSOR_MAX_AGE_MS ||
        !isfinite(s->air_before_mps) || !isfinite(s->air_after_mps) ||
        !isfinite(s->lift_n) || !isfinite(s->drag_n) ||
        s->air_before_mps < 0 || s->air_after_mps < 0 ||
        (s->rpm_valid && (!isfinite(s->fan_rpm) || s->fan_rpm < 0))) return WT_SENSOR;
    return WT_OK;
}

void wt_init(WtControl *c) { memset(c, 0, sizeof(*c)); }
void wt_trip(WtControl *c, WtFault fault)
{
    c->state = WT_FAULT;
    c->fault = fault;
    c->output_percent = 0;
}

void wt_step(WtControl *c, const WtSample *s, uint32_t now, bool ready)
{
    WtFault reason = check(s, now, ready);
    if (c->state == WT_MANUAL) {
        if (reason != WT_OK) wt_trip(c, reason);
        else if ((uint32_t)(now - c->last_command_ms) >= WT_TIMEOUT_MS)
            wt_trip(c, WT_TIMEOUT);
        else if (c->stepped && (uint32_t)(now - c->last_step_ms) > 2u * WT_PERIOD_MS)
            wt_trip(c, WT_TIMING);
    }
    if (c->state != WT_MANUAL) c->output_percent = 0;
    c->last_step_ms = now;
    c->stepped = true;
}

bool wt_command(WtControl *c, const WtSample *s, const WtCommand *cmd,
                uint32_t now, bool ready)
{
    /* STOP always removes output; it never silently clears a latched fault. */
    if (cmd->type == WT_CMD_STOP) {
        c->output_percent = 0;
        if (c->state != WT_FAULT) c->state = WT_STOPPED;
        return true;
    }
    if (cmd->type == WT_CMD_RESET) {
        if (c->state == WT_MANUAL || check(s, now, ready) != WT_OK) return false;
        wt_init(c);
        c->last_step_ms = now;
        c->stepped = true;
        return true;
    }
    if (cmd->type != WT_CMD_MANUAL || !isfinite(cmd->percent) ||
        cmd->percent < 0 || cmd->percent > WT_MAX_PERCENT) return false;
    if (cmd->percent == 0) {
        WtCommand stop = {cmd->id, WT_CMD_STOP, 0};
        return wt_command(c, s, &stop, now, ready);
    }
    if (c->state == WT_FAULT) return false;
    WtFault reason = check(s, now, ready);
    if (reason != WT_OK) { wt_trip(c, reason); return false; }
    c->state = WT_MANUAL;
    c->output_percent = cmd->percent;
    c->last_command_ms = now;
    return true;
}
