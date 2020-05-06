#pragma once

#define BLOCKSIZE 512
#define VOLSIZE 1024 * 1024 * 8
#define BLOCKCOUNT (VOLSIZE / BLOCKSIZE)
#define FREEMAPSIZE (BLOCKCOUNT / sizeof (char *))

typedef enum { false, true } bool;

unsigned get_required_blocks(unsigned size);
void *get_file_data(char *path);
int write_file_data(char *path, void *data, unsigned size);
char **get_args(char *input, int *argc);
unsigned get_file_size(char *path);
char bits2byte(int bits[8]);
int *byte2bits(char byte);
void printbyte(char byte);
