/** @file io.c */
#include "io.h"

#include <ctype.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 512

#ifdef _WIN32
#define PATH_SEPERATOR ('\\')
#include <windows.h>
#define PATH_MAX MAX_PATH
#else
#define PATH_SEPERATOR ('/')
#include <linux/limits.h>
#endif
#define FILE_EXTENSION (".luka")

char *IO_get_file_contents(const char *file_path)
{
    FILE *fp = NULL;
    char *file_contents = NULL;
    int size;

    fp = fopen(file_path, "r");

    if (NULL == fp)
    {
        (void) perror("Couldn't open file");
        goto l_cleanup;
    }

    (void) fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    (void) fseek(fp, 0L, SEEK_SET);

    file_contents = (char *) calloc(sizeof(char), size + 2);
    if (NULL == file_contents)
    {
        (void) perror("Couldn't allocate memory for file contents");
        goto l_cleanup;
    }

    (void) fread(file_contents, size, 1, fp);
    file_contents[size] = EOF;
    file_contents[size + 1] = '\0';

l_cleanup:
    if (NULL != fp)
    {
        (void) fclose(fp);
        fp = NULL;
    }

    return file_contents;
}

size_t io_linelen(const char *buf, int start)
{
    const char *p = buf + start;
    while (*p != '\0')
    {
        if (*p == '\n')
        {
            break;
        }
        ++p;
    }

    return p - (buf + start) + 1;
}

int number_len(int num)
{
    int len = 0;

    if (num < 0)
    {
        ++len;
        num *= -1;
    }

    while (num != 0)
    {
        num /= 10;
        ++len;
    }

    return len;
}

void IO_print_error(const char *file_path, int line, int offset)
{
    char *contents = IO_get_file_contents(file_path);
    size_t i = 0, length = strlen(contents), line_length = 0;
    int cur_line = 0;
    char *err_line = NULL;
    int line_num_length = number_len(line);

    for (i = 0; i < length && cur_line < line - 1; ++i)
    {
        if (contents[i] == '\n')
        {
            cur_line++;
        }
    }

    line_length = io_linelen(contents, i);
    err_line = malloc(line_length + 1);
    if (NULL == err_line)
    {
        (void) perror("Couldn't allocate memory for line");
        goto l_cleanup;
    }
    (void) strncpy(err_line, contents + i, line_length);
    err_line[line_length] = '\0';

    (void) printf(" %d | %s", line, err_line);
    (void) printf(" %*s | %*s^\n", line_num_length, "", offset - 1, "");

l_cleanup:
    if (NULL != contents)
    {
        (void) free(contents);
        contents = NULL;
    }
}

t_return_code IO_copy(const char *original_file_path, const char *new_file_path)
{
    t_return_code status_code = LUKA_UNINITIALIZED;
    FILE *ofp = NULL, *dfp = NULL;
    char buffer[BLOCK_SIZE] = {0};
    size_t bytes_read = 0;

    ofp = fopen(original_file_path, "r");
    if (NULL == ofp)
    {
        (void) perror("Couldn't open file");
        status_code = LUKA_IO_ERROR;
        goto l_cleanup;
    }

    dfp = fopen(new_file_path, "w");
    if (NULL == dfp)
    {
        (void) perror("Couldn't open file");
        status_code = LUKA_IO_ERROR;
        goto l_cleanup;
    }

    while (1)
    {
        bytes_read = fread(buffer, 1, sizeof(buffer), ofp);
        (void) fwrite(buffer, 1, bytes_read, dfp);
        if ((bytes_read < sizeof(buffer)) && (feof(ofp)))
        {
            break;
        }
    }

    status_code = LUKA_SUCCESS;
l_cleanup:
    if (NULL != ofp)
    {
        ON_ERROR(fclose(ofp))
        {
            (void) perror("Couldn't close file");
            status_code = LUKA_IO_ERROR;
        }
    }

    if (NULL != dfp)
    {
        ON_ERROR(fclose(dfp))
        {
            (void) perror("Couldn't close file");
            status_code = LUKA_IO_ERROR;
        }
    }

    return status_code;
}

bool io_is_absolute(const char *path)
{
    return '/' == path[0]
        || (isalpha(path[0]) && (':' == path[1]) && ('\\' == path[2]));
}

/**
 * @brief Verify a path ends with a certain string, including the null byte.
 *
 * @param[in] path the path to check.
 * @param[in] end the string that should appear in the end.
 *
 * @return true if @p path ends with @p end, otherwise false.
 */
bool io_ends_with(const char *path, const char *end)
{
    size_t path_length = strlen(path);
    size_t end_length = strlen(end);
    size_t i = 0;

    if (path_length < end_length)
    {
        return false;
    }

    for (i = 0; i <= end_length; ++i)
    {
        if (path[path_length - i] != end[end_length - i])
        {
            break;
        }
    }

    return (i - 1) == end_length;
}

char *io_append_path(char *path, const char *additional)
{
    size_t new_length = strlen(path) + strlen(additional) + 1;
    path = realloc(path, new_length);
    path = strncat(path, additional, new_length);
    return path;
}

char *IO_resolve_path(const char *requested_path, const char *current_path,
                      bool in_import)
{
    char *path = NULL, *abs_path = NULL;
    const char sep_string[2] = {PATH_SEPERATOR, '\0'};

    if (io_is_absolute(requested_path))
    {
        path = calloc(PATH_MAX, sizeof(char));
        path = strcat(path, requested_path);
        path = strcat(path, FILE_EXTENSION);
        return path;
    }

    path = realpath(current_path, NULL);
    if (in_import)
    {
        path = dirname(path);
    }
    if (path[strlen(path) - 1] != PATH_SEPERATOR)
    {
        path = io_append_path(path, sep_string);
    }

    path = io_append_path(path, requested_path);
    if (!io_ends_with(path, FILE_EXTENSION))
    {
        path = io_append_path(path, FILE_EXTENSION);
    }
    abs_path = realpath(path, abs_path);
    if (NULL == abs_path)
    {
        (void) perror("realpath failed");
        (void) printf("No such file or directory: %s\n", path);
        return path;
    }

    (void) free(path);
    return abs_path;
}
