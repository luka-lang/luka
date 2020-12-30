/** @file io.c */
#include "io.h"

#define BLOCK_SIZE 512

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

    file_contents = (char *)calloc(sizeof(char), size + 2);
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
