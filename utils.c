#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "utils.h"

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
