#include "io.h"

char *IO_get_file_contents(const char *file_path)
{
    FILE *fp = NULL;
    char *file_contents = NULL;
    int size;

    fp = fopen(file_path, "r");

    if (NULL == fp)
    {
        (void) perror("Couldn't open file");
        goto cleanup;
    }

    (void) fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    (void) fseek(fp, 0L, SEEK_SET);

    file_contents = (char *)calloc(sizeof(char), size + 2);
    if (NULL == file_contents)
    {
        (void) perror("Couldn't allocate memory for file contents");
        goto cleanup;
    }

    (void) fread(file_contents, size, 1, fp);
    file_contents[size] = EOF;
    file_contents[size + 1] = '\0';

cleanup:
    if (NULL != fp)
    {
        (void) fclose(fp);
    }

    return file_contents;
}
