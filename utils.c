#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "utils.h"

// Given a filesize in bytes, calcualte it's required blocks to be stored
unsigned get_required_blocks(unsigned size){
	unsigned block_count = round(size / BLOCKSIZE);
	if(block_count == 0) block_count = 1;
	return block_count;
}

// Seek to the end of a file and return it's size
unsigned get_file_size(char *path){
    FILE *buffer;

    buffer = fopen(path, "r");

    // file doesnt exist
    if(!buffer) {
        return 0;
    }

    // Seek to buffer end and get size
    fseek(buffer, 0, SEEK_END);
    int size = ftell(buffer);
    rewind(buffer);

    fclose(buffer);

    return size;
}

// Loads the file & Allocates +  reads a file into memory, return raw data
void *get_file_data(char *path) {
    void *data = NULL;
    int size = get_file_size(path);
    FILE *buffer;

    buffer = fopen(path, "r");

    // Check for error opening buffer
    if(!buffer) {
        printf("Error opening file '%s'\n", path);
        return data;
    }

    // Create space for file in memory
    data = malloc(size);
    if(data == 0){
        printf("Error allocating buffer for '%s'\n", path);
        return data;
    }

    // Read from buffer into new allocated space
    fread(data, 1, size, buffer);

    // Close file buffer
    fclose(buffer);

    printf("Loaded file '%s'\n", path);
    return data;
}


// Return number of args within a given string
int get_arg_count(char *input){
	// Duplicate input so it doesn't screw up the original string
	char *buffer = strdup(input);

	// Loop init
	int i = 0;
	char *s;

	// Iterate over spaces in input
	while((s = strsep(&buffer, " ")) != NULL){
		if(strlen(s) == 0)
			continue;

		i += 1;
	}

	free(buffer);
	return i;
}

// Return an array of arguments split by a space delimeter
char **get_arg_splits(char *input, int arg_count){
	// Duplicate input so it doesn't screw up the original string
	char *buffer = strdup(input);

	// Allocate array of strings for each arg.
	char **arg_splits = malloc((arg_count*sizeof(char *))+1);

	// Finish array with a NULL terminator
	arg_splits[arg_count-1] = NULL;

	// Loop init
	int i = 0;
	char *s;

	// Iterate over spaces in input
	while((s = strsep(&buffer, " ")) != NULL){
		if(strlen(s) == 0)
			continue;

		arg_splits[i] = strdup(s);

		i += 1;
	}

	free(buffer);
	return arg_splits;
}

char **get_args(char *input, int *argc){
    *argc = get_arg_count(input);
    return get_arg_splits(input, *argc);
}

// converts array of binary values into a char byte (0xFF, 0x00, etc)
char bits2byte(int bits[8]){
    char byte = 0;

    for(int i = 0; i < 8; i++){
        byte <<= 1;
        if(bits[i] == 1){
            byte |= 1;
        }
    }

    return byte;
}

// Convert char binary value into bits, returns formatted as array
int *byte2bits(char byte){
	int *bits = malloc(sizeof(int *) * 8);
	for(int i = 0; i < 8; i++){
		if((byte & (1 << i)) > 0){
			bits[7-i] = 1;
		}else{
			bits[7-i] = 0;
		}
	}
	return bits;
}

void printbyte(char byte){
    int *bits = byte2bits(byte);
    for(int i = 0; i < 8; i++) printf("%d", bits[i]);
    printf("\n");
}
