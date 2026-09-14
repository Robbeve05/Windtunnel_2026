#include "windtunnel.h"
#include "protocol.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static WtSample fresh(uint32_t now)
{
    WtSample s = {0};
    s.sampled_ms = now;
    s.valid = s.door_closed = s.estop_ok = true;
    return s;
}
int main(void)
{
    WtControl c;
    WtCommand manual = {1, WT_CMD_MANUAL, 20}, stop = {2, WT_CMD_STOP, 0};
    WtCommand reset = {3, WT_CMD_RESET, 0}, parsed;
    WtSample s = fresh(0);
    char line[256];
    wt_init(&c);
    assert(c.state == WT_STOPPED && c.output_percent == 0);
    assert(!wt_command(&c, &s, &manual, 0, false));
    assert(c.fault == WT_NOT_READY && c.output_percent == 0);
    assert(wt_command(&c, &s, &stop, 0, false));
    assert(c.state == WT_FAULT);
    assert(wt_command(&c, &s, &reset, 0, true));
    assert(wt_command(&c, &s, &manual, 0, true));
    for (uint32_t t = 10; t <= 1000; t += 10) {
        s = fresh(t);
        wt_step(&c, &s, t, true);
    }
    assert(c.fault == WT_TIMEOUT && c.output_percent == 0);
    assert(!wt_command(&c, &s, &manual, 1000, true));
    assert(wt_command(&c, &s, &reset, 1000, true));
    assert(wt_command(&c, &s, &manual, 1000, true));
    s.door_closed = false;
    wt_step(&c, &s, 1010, true);
    assert(c.fault == WT_INTERLOCK && c.output_percent == 0);
    assert(!wt_command(&c, &s, &reset, 1010, true));
    s = fresh(1020);
    assert(wt_command(&c, &s, &reset, 1020, true));
    assert(wt_command(&c, &s, &manual, 1020, true));
    s.air_before_mps = NAN;
    wt_step(&c, &s, 1030, true);
    assert(c.fault == WT_SENSOR);
    s = fresh(1040);
    assert(wt_command(&c, &s, &reset, 1040, true));
    manual.percent = INFINITY;
    assert(!wt_command(&c, &s, &manual, 1040, true));
    manual.percent = 31;
    assert(!wt_command(&c, &s, &manual, 1040, true));
    manual.percent = 20;
    assert(wt_command(&c, &s, &manual, 1040, true));
    s = fresh(1100);
    wt_step(&c, &s, 1100, true);
    assert(c.fault == WT_TIMING && c.output_percent == 0);
    assert(wt_command(&c, &s, &reset, 1100, true));
    assert(wt_command(&c, &s, &manual, 1100, true));
    wt_step(&c, &s, 1201, true);
    assert(c.fault == WT_SENSOR);
    /* Unsigned millisecond rollover must preserve watchdog timing. */
    wt_init(&c);
    s = fresh(UINT32_MAX - 5u);
    assert(wt_command(&c, &s, &manual, s.sampled_ms, true));
    wt_step(&c, &s, s.sampled_ms, true);
    s = fresh(4);
    wt_step(&c, &s, 4, true);
    assert(c.state == WT_MANUAL);
    assert(wt_parse_command("WT1,C,4294967295,MANUAL,20.5", &parsed));
    assert(parsed.id == UINT32_MAX && parsed.percent == 20.5f);
    const char *bad[] = {"WT1,C,-1,STOP", "WT1,C,4294967296,STOP", "WT1,C,1,STOP,x",
        "WT1,C,1,MANUAL,nan", "WT1,C,1,MANUAL,31", "WT1,C,1,MANUAL,20x", "garbage"};
    for (unsigned i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i)
        assert(!wt_parse_command(bad[i], &parsed));
    assert(wt_format_sample(line, sizeof(line), &c, &s) > 0);
    assert(strncmp(line, "WT1,S,4,1,0,20.000", 18) == 0);
    assert(wt_format_sample(line, 5, &c, &s) == 0);
    puts("Core: boot, stop, reset, interlocks, watchdog, invalid/stale sensors, timing, rollover and protocol passed.");
    /* Ten simulated seconds: acquisition -> command -> control -> plant. */
    wt_init(&c);
    float velocity = 0;
    for (uint32_t t = 0; t < 10000; t += WT_PERIOD_MS) {
        s = fresh(t);
        s.air_before_mps = velocity;
        wt_step(&c, &s, t, true);
        if (t % 200 == 0) assert(wt_command(&c, &s, &manual, t, true));
        velocity += (c.output_percent * .4f - velocity) * .01f;
        assert(c.state == WT_MANUAL);
    }
    assert(velocity > 7.9f && velocity < 8.1f);
    printf("Simulated 100 Hz loop: output %.1f%%, airflow %.2f m/s (illustrative model).\n", c.output_percent, velocity);
    return 0;
}
