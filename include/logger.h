/** @file logger.h */
#ifndef LUKA_LOGGER_H
#define LUKA_LOGGER_H

#include <stdarg.h>
#include <stdio.h>

#include "defs.h"
#include "io.h"

typedef struct
{
    FILE *fp;         /**< A file pointer to the logger file. */
    char *file_path;  /**< The path of the logger file */
    size_t verbosity; /**< The verbosity of the logger */
} t_logger;

#define L_DEBUG   "DEBUG"   /**< Log string for the DEBUG level */
#define L_ERROR   "ERROR"   /**< Log string for the ERROR level*/
#define L_INFO    "INFO"    /**< Log string for the INFO level*/
#define L_WARNING "WARNING" /**< Log string for the WARNING level*/

/**
 * @brief Initializes a new logger.
 *
 * @param[in] file_path the path to log to.
 * @param[in] verbosity how verbose should the logger be.
 *
 * @return a new logger that will log to file_path with the given verbosity.
 */
t_logger *LOGGER_initialize(char *file_path, size_t verbosity);

/**
 * @brief Log a new message to the log file.
 *
 * @param[in] logger the logger to log with.
 * @param[in] level the severity level of the log message.
 * @param[in] format the format of the log message.
 * @param[in] ... additional arguments to the log formatter.
 */
void LOGGER_log(t_logger *logger, const char *level, const char *format, ...);

/**
 * @brief Log a new message to the log file.
 *
 * @param[in] logger the logger to log with.
 * @param[in] level the severity level of the log message.
 * @param[in] token the token that contains the location of the logged message.
 * @param[in] format the format of the log message.
 * @param[in] ... additional arguments to the log formatter.
 */
#define LOGGER_LOG_LOC(logger, level, token, format, ...)                      \
    do                                                                         \
    {                                                                          \
        if (NULL != token)                                                     \
        {                                                                      \
            (void) LOGGER_log(logger, level,                                   \
                              "%s:%ld:%ld: error: ", token->file_path,         \
                              token->line, token->offset);                     \
        }                                                                      \
        (void) LOGGER_log(logger, level, format, __VA_ARGS__);                 \
        if (NULL != token)                                                     \
        {                                                                      \
            (void) IO_print_error(token->file_path, token->line,               \
                                  token->offset);                              \
        }                                                                      \
    } while (0)

/**
 * @brief Deallocates all memory allocated by @p logger.
 *
 * @param[in] logger the logger to free.
 */
void LOGGER_free(t_logger *logger);

#endif // LUKA_LOGGER_H
