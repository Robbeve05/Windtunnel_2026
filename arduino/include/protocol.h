#ifndef WT_PROTOCOL_H
#define WT_PROTOCOL_H
#include "windtunnel.h"
#include <stddef.h>
#define WT_LINE_SIZE 256u
bool wt_parse_command(const char *line, WtCommand *command);
size_t wt_format_sample(char *out, size_t size, const WtControl *c, const WtSample *s);
#endif
