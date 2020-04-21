#pragma once

typedef enum { false, true } bool;

void *get_file_data(char *path);
char **get_args(char *input, int *argc);
unsigned get_file_size(char *path);
