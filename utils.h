#pragma once

#define BLOCKSIZE 512
#define VOLSIZE 512 * 100

void *get_file_data(char *path);
char **get_args(char *input, int *argc);
unsigned get_file_size(char *path);
