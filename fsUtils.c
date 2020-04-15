#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "fsUtils.h"

// Loads the file & Allocates +  reads a file into memory, return raw data
void *read_file(char *path) {
    void *data = NULL;
    int size = 0;
    FILE *buffer;

    buffer = fopen(path, "r");

    // Check for error opening buffer
    if(!buffer) {
        printf("Error opening file '%s'\n", path);
        return data;
    }

    // Seek to buffer end and get size
    fseek(buffer, 0, SEEK_END);
    size = ftell(buffer);
    rewind(buffer);

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
