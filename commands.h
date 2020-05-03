#pragma once

#include <stdlib.h>
#include "utils.h"

// Metadata about the file
typedef struct {
	unsigned long time_created;
} Nugget;

typedef struct {
	long block_data; 	// ptr to data / directory start block
	long block_next;	// ptr to next entry in directory
	unsigned short size; 	// size (bytes) of the associated file / dir
	char name[256];
	int is_dir;
	Nugget info;
} Entry;

typedef struct{
	long block_start; // block of first dir in the chain
	long block_dir; // current block where dir is stored 
	char name[256];
} Directory;

typedef struct{
	long ptr_root; 		// Pointer to root directory
	long ptr_freemap;	// Pointer to freemap blocks
	long len_freemap;	// Length of freemap blocks
} Superblock;

// DIR functions
unsigned dir_create (char *name, Directory *container);	// Create a new dir within a dir struct
unsigned dir_move (Directory *src, Directory *dest);	// Move an existing dir to another location 
Directory *dir_load (unsigned blk_start);	// Load a dir based on it's block location
int dir_list (Directory *dir);		// List all entries contained within a dir 
unsigned dir_find_entry (char *name, Directory *dir);	// Search for an Entry in a given dir, return NULL if doesnt exist
unsigned dir_find_end (Directory *dir);	// Search for an Entry in a given dir, return NULL if doesnt exist

// NAVIGATION functions
int fs_change_dir (char *name);		// Search for matching name within current directory, change current directory
Directory *fs_get_cur_dir ();		// Returns Directory object representing current dir (default: root)

// FILE functions
int file_add_file_external (char *filepath);		// Copy an external file into current dir
int file_write_file_external (char *name);		// Write internal file into external dir 
void *file_read (Directory *dir, char *name);		// Read the data of a file based on it's container dir and name
int file_rm (Directory *dir, char *name);		// Remove a file entry based on it's container dir and name
int file_move (Directory *src, Directory *dest, char *name);	// Move a file with matching 'name', within dir 'src', into dir 'dest'

// FS core functions
int fs_start (char *filename);	// Start filesystem. Initialize if non-existant.
void fs_close ();		// Close filesystem, free all globals
