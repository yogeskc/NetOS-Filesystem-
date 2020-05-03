#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "freemap.h"
#include "low.h"
#include "utils.h"

// Global fs vars
Superblock *g_super = NULL;
Directory *g_cur_dir = NULL;

unsigned dir_create_root(){	
	// Allocate root directory and (..) entry
	Directory *root = malloc(sizeof(Directory));
	Entry *parent = malloc(sizeof(Entry));

	// Mark in freemap
	unsigned root_start = freemap_find_freespace(1);
	freemap_set(1, 1, root_start);
	unsigned parent_start = freemap_find_freespace(1);
	freemap_set(1, 1, parent_start);

	// Fill out root data
	strcpy(root->name, "root");
	root->block_start = parent_start;
	root->block_dir = root_start;

	// Fill out (..) data
	strcpy(parent->name, "..");
	parent->block_data = root_start;
	parent->block_next = -1;
	parent->is_dir = 1;

	// Write to disk
	LBAwrite(root, 1, root_start);
	LBAwrite(parent, 1, parent_start);
	
	// Update freemap
	freemap_save();

	// free buffers
	free(root);
	free(parent);

	return root_start;
}

unsigned dir_create(char *name, Directory *container){
	printf("Creating dir %s in container %s\n", name, container->name); 
	// check if dir already exists
	if(dir_find_entry (name, container) != -1){
		printf("Directory %s already exists!\n", name);
		return -1;
	}

	// Allocate directory and link entries
	Entry *link = malloc(sizeof(Entry));
	Entry *parent = malloc(sizeof(Entry));
	Directory *dir = malloc(sizeof(Directory));

	// Mark in freemap
	unsigned link_start = freemap_find_freespace(1);
	freemap_set(1,1, link_start);

	unsigned dir_start = freemap_find_freespace(1);
	freemap_set(1, 1, dir_start);

	unsigned parent_start = freemap_find_freespace(1);
	freemap_set(1, 1, parent_start);

	// Fill out link data
	strcpy(link->name, name);
	link->block_data = dir_start;
	link->block_next = -1;
	link->is_dir = 1;

	// Fill out (..) data
	strcpy(parent->name, "..");
	parent->block_data = container->block_dir;
	parent->block_next = -1;
	parent->is_dir = 1;
	
	// Fill out directory data
	strcpy(dir->name, name);
	dir->block_start = parent_start;
	dir->block_dir = dir_start;
	
	// Write to disk
	LBAwrite(link, 1, link_start);
	LBAwrite(parent, 1, parent_start);
	LBAwrite(dir, 1, dir_start);

	// free buffers
	free(link);
	free(parent);
	free(dir);

	// Append link to container end 
	unsigned cnt_end_ptr = dir_find_end(container);
	Entry *cnt_end = malloc(BLOCKSIZE);

	LBAread(cnt_end, 1, cnt_end_ptr);
	cnt_end->block_next = link_start;
	LBAwrite(cnt_end, 1, cnt_end_ptr);

	free(cnt_end);

	// Update freemap
	freemap_save();

	return dir_start;
}

int dir_list(Directory *dir){
	printf("Listing DIR %s\n", dir->name);

	Entry *iter = malloc(BLOCKSIZE);

	// Load first dir & print it's name
	LBAread(iter, 1, dir->block_start);
	printf("- %s\n", iter->name);

	// Continue loading dirs until end is reached
	while(iter->block_next != -1){
		LBAread(iter, 1, iter->block_next);
		printf("- %s\n", iter->name);
	}

	free(iter);

	return 0;
}

// Search for an Entry in a given dir, return NULL if doesnt exist
unsigned dir_find_entry (char *name, Directory *dir){
	Entry *iter = malloc(BLOCKSIZE);

	// Load first dir & print it's name
	LBAread(iter, 1, dir->block_start);
	unsigned ptr = dir->block_start;

	// Check if names match
	if(strcmp(iter->name, name) == 0){
		free(iter);
		return ptr;
	}

	// Continue loading dirs until end is reached
	while(iter->block_next != -1){
		ptr = iter->block_next;
		LBAread(iter, 1, iter->block_next);

		// Check if names match
		if(strcmp(iter->name, name) == 0){
			free(iter);
			return ptr;
		}
	}

	// Dir not found...
	free(iter);
	return ptr;
}

// Return final entry in directory
unsigned dir_find_end(Directory *dir){
	Entry *iter = malloc(BLOCKSIZE);

	// Load first dir & print it's name
	LBAread(iter, 1, dir->block_start);

	unsigned ptr = dir->block_start;

	// Continue loading dirs until end is reached
	while(iter->block_next != -1){
		ptr = iter->block_next;
		LBAread(iter, 1, iter->block_next);
	}

	// return ptr to final entry
	free(iter);
	return ptr;
}

// Create a blank filesystem 
int fs_create(){
	printf("Initialize core system\n");
	
	// Create a blank freemap, then mark it.
	freemap_create();

	// Allocate and mark superblock
	g_super = malloc(sizeof(Superblock));
	freemap_set(1, 1, 0);

	// Write root dir to disk (after freemap)
	unsigned root_start = dir_create_root();
	printf("Wrote root dir at block %d\n", root_start);

	// Create superblock (block 0)
	g_super->ptr_freemap = freemap_get_start();
	g_super->len_freemap = freemap_get_len();
	g_super->ptr_root = root_start;

	// Write core system to disk
	printf("Wrote superblock\n");
	LBAwrite(g_super, 1, 0);

	printf("Wrote %d empty freemap blocks\n", freemap_get_len());
	freemap_save();

	return 0;
}

int fs_load_globals(){
	// Allocate / Load superblock
	g_super = malloc(BLOCKSIZE);
	LBAread(g_super, 1, 0);

	// Allocate / Load freemap 
	freemap_load(g_super->ptr_freemap, g_super->len_freemap);

	// Allocate / Load cur_dir
	g_cur_dir = malloc(BLOCKSIZE);
	LBAread(g_cur_dir, 1, g_super->ptr_root);
}

// Locate 'filename' and start filesystem off of it. Initialize the filesystem if necessary.
int fs_start(char *filename){
	// Create filesystem file
	uint64_t blockSize = BLOCKSIZE;
	uint64_t volumeSize = VOLSIZE;

	int part_status = startPartitionSystem(filename, &volumeSize, &blockSize);
	//printf("Partition system started with status %d\n", part_status);

	// Error on FS read from disk
	if(part_status != 2 && part_status != 0){
		printf("Error creating/loading the filesystem file!\n");
		return part_status;
	}

	// If filesystem is brand new, create core structure 
	if (part_status == 2){
		part_status = fs_create();
	}

	// If filesystem already exists, load core structure
	if(part_status == 0){
		fs_load_globals();
	}

	// Load core structure (freemap & superblock)
	printf("Loaded core filesystem vars.\n");
	
	return part_status;
}

void fs_close(){
	closePartitionSystem();

	freemap_cleanup();
	free(g_super);
	free(g_cur_dir);
}

// Change the current directory
int fs_change_dir (char *name){
	// Find matching entry within current dir
	unsigned target_ptr = dir_find_entry(name, g_cur_dir);

	if(target_ptr == -1){
		printf("Cannot cd to %s, directory not found\n", name);
		return -1;
	}

	Entry *target = malloc(BLOCKSIZE);
	LBAread(target, 1, target_ptr);

	if(target->is_dir == 0){
		printf("Cannot cd to %s, target is not a directory\n", target->name);
		free(target);
		return -1;
	}

	printf("Loading dir at %d\n", target->block_data);
	LBAread(g_cur_dir, 1, target->block_data);
	free(target);
}

// Returns Directory object representing current dir (default: root)
Directory *fs_get_cur_dir (){
	return g_cur_dir;
}
