#pragma once

#include <stdlib.h>
#include "utils.h"

#define BLOCKSIZE 512
#define VOLSIZE 1024 * 1024 * 8
#define BLOCKCOUNT (VOLSIZE / BLOCKSIZE)
#define FREEMAPSIZE (BLOCKCOUNT / sizeof(char *))

// Metadata about the file
typedef struct {
	unsigned long time_created;
} Nugget;

typedef struct {
	long block_data; 	// ptr to data / directory start block
	long block_next;	// ptr to next entry in directory
	unsigned short size; 	// size (bytes) of the associated file / dir
	char name[256];
	Nugget info;
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
int fs_start(char *filename);	// Start filesystem. Initialize if non-existant.
void fs_close();

// FILE functions
// todo ...
int fs_add_file(char *filepath, unsigned blk_start);	
void *fs_read_file(unsigned blk_start);

// DIR functions
//void fs_create_dir(char *name, unsigned blk_start);	// Creates a new directory in a specified location
Directory *dir_load(unsigned blk_start);
int dir_list(Directory *dir);

// PATH functions
Entry fs_resolve_path(char *path);	// Return an associated Entry with a given path

// FREEMAP functions
void freemap_load();		// TODO - Search the fs for where the freemap is stored, then load it into memory
void freemap_set(bool taken, unsigned blk_len, unsigned blk_start); 	// iterate over range on freemap and set bits to 1 or 0
unsigned freemap_find_freespace(unsigned blk_len);						// search freemap for a contiguous free space of 'blk_len'
