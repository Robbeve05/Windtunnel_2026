#include "protocol.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool wt_parse_command(const char *line, WtCommand *cmd)
{
    char *end;
    unsigned long id;
    if (strncmp(line, "WT1,C,", 6) != 0) return false;
    line += 6;
    if (*line < '0' || *line > '9') return false;
    errno = 0;
    id = strtoul(line, &end, 10);
    if (errno || id > UINT32_MAX || *end != ',') return false;
    cmd->id = (uint32_t)id;
    line = end + 1;
    cmd->percent = 0;
    if (!strcmp(line, "STOP")) cmd->type = WT_CMD_STOP;
    else if (!strcmp(line, "RESET")) cmd->type = WT_CMD_RESET;
    else if (!strncmp(line, "MANUAL,", 7)) {
        line += 7;
        if (*line < '0' || *line > '9') return false;
        errno = 0;
        cmd->percent = strtof(line, &end);
        if (errno || end == line || *end || !isfinite(cmd->percent) ||
            cmd->percent < 0 || cmd->percent > WT_MAX_PERCENT) return false;
        cmd->type = WT_CMD_MANUAL;
    } else return false;
    return true;
}

size_t wt_format_sample(char *out, size_t size, const WtControl *c, const WtSample *s)
{
    int n = snprintf(out, size,
        "WT1,S,%lu,%d,%d,%.3f,%d,%d,%d,%.4f,%.4f,%.4f,%.4f,%d,%.2f\n",
        (unsigned long)s->sampled_ms, (int)c->state, (int)c->fault,
        (double)c->output_percent, s->valid, s->door_closed, s->estop_ok,
        (double)s->air_before_mps, (double)s->air_after_mps,
        (double)s->lift_n, (double)s->drag_n, s->rpm_valid, (double)s->fan_rpm);
    return n > 0 && (size_t)n < size ? (size_t)n : 0;
}
