#pragma once

#include <stdlib.h>
#include "utils.h"

#define BLOCKSIZE 512
#define BLOCKCOUNT 100
#define BLOCKTOTAL BLOCKSIZE * BLOCKCOUNT

typedef struct {
	long block_start;
} Nugget;

typedef struct {
	Nugget nug_cur;			 	// updown in the list
    Nugget nug_next; 			// down in the list
	unsigned short size; 		// size (in bytes) of the associated file / dir
	char name[256];
} Entry;

typedef struct{
	long block_start;
	long ptr_files[];
} Directory;

// FILE functions
int fs_add_file(char *filepath, unsigned blk_start);	// todo - write new file somewhere in the fs, and append new entry to a Directory arr
void *fs_read_file(unsigned blk_start);					// todo - read file based on a path. index in a Directory

// DIR functions
void fs_create_dir(char *name, unsigned blk_start);		// Creates a new directory in a specified

// PATH functions
Entry fs_resolve_path(char *path);		// Return an associated Entry with a given path

// FREEMAP functions
//void freemap_init();
//void freemap_cleanup();
void freemap_set(char *freemap, bool taken, unsigned blk_start, unsigned blk_end); 	// iterate over range on freemap and set bits to 1 or 0
unsigned freemap_find_freespace(char *freemap, unsigned blk_len); 									// iterate over freemap and find a contiguous space of blk_len
