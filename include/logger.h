#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdarg.h>
#include <stdio.h>

#include "defs.h"

typedef struct {
    FILE *fp;
    char *file_path;
} t_logger;

#define L_DEBUG "DEBUG"
#define L_ERROR "ERROR"
#define L_INFO "INFO"
#define L_WARNING "WARNING"

t_logger *LOGGER_initialize(char *file_path);

void LOGGER_log(t_logger *logger, const char *level, const char *format, ...);

void LOGGER_free(t_logger *logger);

#endif // __LOGGER_H__
