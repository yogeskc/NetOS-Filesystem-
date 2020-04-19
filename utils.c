#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "utils.h"
//test
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
