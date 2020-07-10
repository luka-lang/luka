#include "../include/io.h"

char *get_file_contents(const char *file_path) {
  FILE *fp = NULL;
  char *file_contents = NULL;
  int size;

  fp = fopen(file_path, "r");

  if (NULL == fp) {
    perror("Couldn't open file");
    goto get_file_contents_exit;
  }

  fseek(fp, 0L, SEEK_END);
  size = ftell(fp);
  fseek(fp, 0L, SEEK_SET);

  file_contents = (char *)calloc(sizeof(char), size + 2);
  if (NULL == file_contents) {
    perror("Couldn't allocate memory for file contents");
    goto get_file_contents_exit;
  }

  fread(file_contents, size, 1, fp);
  file_contents[size] = EOF;
  file_contents[size + 1] = '\0';

get_file_contents_exit:
  if (NULL != fp) {
    fclose(fp);
  }

  return file_contents;
}
