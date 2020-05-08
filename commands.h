#pragma once

#include <stdlib.h>
#include "utils.h"

// Metadata about a given entry
typedef struct {
	unsigned time_created;
} Nugget;

// Data structure which points to a directory or file
typedef struct {
	unsigned block_data; 	// ptr to data / directory start block
	unsigned block_next;	// ptr to next entry in directory
	unsigned size; 			// size (bytes) of the associated file / dir
	char name[256];
	int is_dir;
	Nugget info;
} Entry;

// Data structure which points to the beginning of a directory entry chain
typedef struct{
	unsigned block_start; // block of first dir in the chain
	char name[256];
} Directory;

// Data structure for holding global filesystem info
typedef struct{
	unsigned ptr_root; 		// Pointer to root directory
	unsigned ptr_freemap;	// Pointer to freemap blocks
	unsigned len_freemap;	// Length of freemap blocks
} Superblock;

// DIR functions

// Create a new directory 
// name - name of new directory to create
// dir_ptr - block location of container directory
// return - new block location of created directory
unsigned dir_create (char *name, unsigned container_ptr);	
unsigned dir_create_root ();	

// Move a directory into another
// src - block location of directory to modify
// dest - block location of directory to move into
// return - new block location of src directory
unsigned dir_move (char *name_src, char *path_dest, unsigned container_ptr);

// Delete a directory and all entries within it
// name - name of directory to delete
// dir - block location of container directory
// return - 0 on success, -1 otherwise
int dir_rm(char *name, unsigned container_ptr);

// Load a pointer to a directory into a Directory struct
// dir_ptr - block location of target directory 
// return - A malloc'd struct filled with the target directory's data
Directory *dir_load (unsigned dir_ptr);			

// Load a pointer to a directory into a Entry struct
// entry_ptr - block location of target directory 
// return - A malloc'd struct filled with the target directory's data
Entry *entry_load (unsigned entry_ptr);			

// Point the final entry in a directory chain to a new entry.
int entry_chain_append(unsigned container_ptr, unsigned entry_ptr);

// List alll entries within a directory
// dir_ptr - block location of target directory
// return - 0 if success, -1 otherwise 
int dir_list (unsigned dir_ptr);
int dir_tree (unsigned dir_ptr, int level);

// Search a directory for a given entry matching "name"
// name - string to search for
// dir - block location of directory to search
// return - pointer to search result, -1 if doesn't exist
unsigned dir_find_entry (char *name, unsigned dir_ptr, bool before);	

// Follow the chain of entries within a directory until the end is reached
// dir - block location of directory to iterate 
// return - pointer to final element
unsigned dir_find_end (unsigned dir);	

// In reference to a container dir, advance into the next directory
// dir - directory to start from
// name - directory to advance into 
// return - pointer to target dir 
unsigned dir_advance(char *name, unsigned dir_ptr);

// Return entry associated with a path
// dir - starting dir. Where the path reference starts from
unsigned resolve_path(char *path, unsigned dir, bool before);

// FILE functions

// Search for an entry within a directory, return it's raw associated data
// name - name of file to search for
// dir - block location of directory to search
void *file_read (char *name, unsigned dir_ptr);

// Search for an entry within a directory, delete it. MUST BE A FILE
// name - name of file to search for
// dir - block location of directory to search
int file_rm (char *name, unsigned dir_ptr);

// Search for entry within a directory, move it to another directory
// name - name of file to search for
// src - block location of directory to search
// dest - block location of directory to move file into
int file_move (char *name, unsigned src, unsigned dest);	

// Copy an external file into an internal directory
// filepath - path to file in regular filesystem
// dir - block location of directory to copy into
// return - 0 on success, -1 otherwise
int exfile_add (char *path_ext, unsigned dir);

// Search for an internal file within a given directory, then write it out to the external filesystem
// filepath - Path to where the internal file will be placed in the external filesystem
// name - name of file to copy out
// dir - block location of directory to search 
// return - 0 on success, -1 otherwise
int exfile_write (char *path_int, char *path_ext, unsigned dir);

// NAVIGATION functions

// Resolve the input path, then move into that directory
// name - directory to search for
// return - 0 on success, -1 otherwise
int fs_change_dir (char *path);		

// Returns block location of current directory (default: root)
unsigned fs_get_cur_dir ();

// FS core functions

// Start the filesystem. Create empty if non-existant. Otherwise, load the existing data.
// filename - filepath to filesystem data file.
// return - 0 on success, -1 otherwise
int fs_start (char *filename);

// Close filesystem, free all globals
void fs_close ();
