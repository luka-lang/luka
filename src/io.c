/** @file io.c */
#include "io.h"

#include <ctype.h>
#include <libgen.h>

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
    }

    return file_contents;
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

char *IO_resolve_path(const char *requested_path, const char *current_path)
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
    path = dirname(path);
    if (path[strlen(path) - 1] != PATH_SEPERATOR)
    {
        path = strcat(path, sep_string);
    }
    path = strcat(path, requested_path);
    path = strcat(path, FILE_EXTENSION);
    abs_path = realpath(path, abs_path);
    if (NULL == abs_path)
    {
        return path;
    }

    (void) free(path);
    return abs_path;
}
