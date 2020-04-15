#pragma once

#include <stdint.h>

// This is our nugget struct
typedef struct {
	long ptr_data_start;
} Nugget;

typedef struct {
	Nugget nug_cur;		 //updown in the list
    Nugget nug_next; 		//down in the list
	// number of blocks it takes up
    unsigned short d_reclen;
	char d_name[256];
} Entry;

void copy_file(char *filepath, unsigned data_start, uint64_t blockSize);
