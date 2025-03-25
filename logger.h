#ifndef PROJEKT1_LOGGER_H
#define PROJEKT1_LOGGER_H

#include <stdio.h>

#define MIN 1
#define STANDARD 2
#define MAX 3

#define SIG_DUMP (SIGRTMIN)
#define SIG_LOG_TOGGLE (SIGRTMIN + 1)
#define SIG_LOG_LEVEL (SIGRTMIN + 2)

int init_logger();
int close_logger();
int write_log(int level, const char *fmt, ...);
void set_dump_callback(void (*callback)(FILE *));

#endif
