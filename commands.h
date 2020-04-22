#pragma once

#include <stdlib.h>
#include "utils.h"

#define BLOCKSIZE 512
#define VOLSIZE 1024 * 1024 * 8
#define BLOCKCOUNT (VOLSIZE / BLOCKSIZE)
#define FREEMAPSIZE (BLOCKCOUNT / sizeof(char *))

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
} Directory;

typedef struct{
	long ptr_root; 		// Pointer to root directory
	long ptr_freemap;	// Pointer to freemap blocks
	long len_freemap;	// Length of freemap blocks
} Superblock;

// FS functions
int fs_start(char *filename);			// Start filesystem. Initialize if non-existant.

// FILE functions
int fs_add_file(char *filepath, unsigned blk_start);	// todo - write new file somewhere in the fs, and append new entry to a Directory arr
void *fs_read_file(unsigned blk_start);					// todo - read file based on a path. index in a Directory

// DIR functions
//void fs_create_dir(char *name, unsigned blk_start);		// Creates a new directory in a specified location

// PATH functions
Entry fs_resolve_path(char *path);		// Return an associated Entry with a given path

// FREEMAP functions
void freemap_init();		// Create a new, 0'd out freemap. Only run on init filesystem creation
void freemap_load();		// TODO - Search the fs for where the freemap is stored, then load it into memory
void freemap_cleanup();		// Clear memory of current freemap
void freemap_set(bool taken, unsigned blk_start, unsigned blk_end); 	// iterate over range on freemap and set bits to 1 or 0
unsigned freemap_find_freespace(unsigned blk_len);						// search freemap for a contiguous free space of 'blk_len'
