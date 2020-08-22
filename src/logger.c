#include "logger.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

t_logger *LOGGER_initialize(char *file_path)
{
    t_logger *logger = NULL;
    FILE *fp = NULL;

    logger = malloc(sizeof(t_logger));
    if (NULL == logger)
    {
        goto cleanup;
    }

    logger->file_path = file_path;
    logger->fp = fopen(file_path, "a");
    if (NULL == logger->fp)
    {
        goto cleanup;
    }

    return logger;

cleanup:
    if (NULL != fp)
    {
        (void) fclose(fp);
        fp = NULL;
    }

    if (NULL != logger)
    {
        (void) free(logger);
        logger = NULL;
    }

    return NULL;

}

void LOGGER_log(t_logger *logger, const char *level, const char *format, ...)
{
    va_list args;
    time_t now = {0};
    char *time_string = NULL;

    if ((NULL != logger) && (NULL != logger->fp))
    {
        (void) va_start(args, format);

        (void) time(&now);
        time_string = ctime(&now);
        time_string[strlen(time_string) - 1] = '\0';
        (void) fprintf(logger->fp, "%s [%s]: ", time_string, level);
        (void) vfprintf(logger->fp, format, args);
        (void) fflush(logger->fp);
        (void) va_end(args);
    }
    else
    {
        (void) fprintf(stderr, "Logger is not initialized.\n");
    }

}

void LOGGER_free(t_logger *logger)
{
    if (NULL != logger)
    {
        if (NULL != logger->fp)
        {
            (void) fclose(logger->fp);
            logger->fp = NULL;
        }

        (void) free(logger);
        logger = NULL;
    }
}
