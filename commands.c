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
unsigned g_cur_dir = -1;

unsigned dir_create_root(){	
	// Allocate root directory and (..) entry
	Directory *root = malloc(sizeof(Directory));
	Entry *parent = malloc(sizeof(Entry));

	// Mark in freemap
	unsigned root_start = freemap_find_freespace(1, true);
	unsigned parent_start = freemap_find_freespace(1, true);

	// Fill out root data
	strcpy(root->name, "root");
	root->block_start = parent_start;

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

unsigned dir_create(char *name, unsigned container_ptr){
	// check if dir already exists
	if(dir_find_entry (name, container_ptr) != -1){
		printf("Directory %s already exists!\n", name);
		return -1;
	}

	// Allocate directory and link entries
	Entry *link = malloc(sizeof(Entry));
	Entry *parent = malloc(sizeof(Entry));
	Directory *dir = malloc(sizeof(Directory));

	// Mark in freemap
	unsigned link_start = freemap_find_freespace(1, true);
	unsigned dir_start = freemap_find_freespace(1, true);
	unsigned parent_start = freemap_find_freespace(1, true);

	// Fill out link data
	strcpy(link->name, name);
	link->block_data = dir_start;
	link->block_next = -1;
	link->is_dir = 1;

	// Fill out (..) data
	strcpy(parent->name, "..");
	parent->block_data = container_ptr;
	parent->block_next = -1;
	parent->is_dir = 1;
	
	// Fill out directory data
	strcpy(dir->name, name);
	dir->block_start = parent_start;
	
	// Write to disk
	LBAwrite(link, 1, link_start);
	LBAwrite(parent, 1, parent_start);
	LBAwrite(dir, 1, dir_start);


	// free buffers
	free(link);
	free(parent);
	free(dir);

	// Load directory associated with container ptr
	Directory *container = dir_load(container_ptr);
	printf("Created dir %s:%d in container %s:%d\n", name, dir_start, container->name, container_ptr); 
	free(container);

	// Append link to container end 
	unsigned cnt_end_ptr = dir_find_end(container_ptr);
	Entry *cnt_end = malloc(BLOCKSIZE);

	LBAread(cnt_end, 1, cnt_end_ptr);
	cnt_end->block_next = link_start;
	LBAwrite(cnt_end, 1, cnt_end_ptr);

	free(cnt_end);

	// Update freemap
	freemap_save();

	return dir_start;
}

unsigned dir_move(char *name, unsigned container_ptr, unsigned dest_ptr){
	/*/ Load directory associated with dest ptr
	Directory *container = dir_load(container_ptr);
	Directory *dest = dir_load(dest_ptr);

	// Remove src dir from container

	// Append src dir to dest end 
	unsigned dest_end_ptr = dir_find_end(dest_ptr);
	Entry *dest_end = entry_load(dest_end_ptr);
	
	cnt_end->block_next = link_start;
	LBAwrite(cnt_end, 1, cnt_end_ptr);

	free(cnt_end);*/
	return -1;
}

int dir_rm(char *name, unsigned container_ptr){
	return -1;
}

int dir_list(unsigned dir_ptr){
	// Load directory associated with container ptr
	Directory *dir = malloc(BLOCKSIZE);
	LBAread(dir, 1, dir_ptr);

	printf("Listing DIR %s:%d\n", dir->name, dir_ptr);

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
	free(dir);

	return 0;
}

unsigned dir_find_entry (char *name, unsigned dir_ptr){
	Directory *dir = dir_load(dir_ptr);

	// Load first entry in dir
	Entry *iter = malloc(BLOCKSIZE);
	LBAread(iter, 1, dir->block_start);
	unsigned ptr = dir->block_start;

	while(strcmp(iter->name, name) != 0){
		if(iter->block_next == -1){
			ptr = -1;
			break;
		}

		// found match
		if(strcmp(iter->name, name) == 0){
			break;
		}

		ptr = iter->block_next;
		LBAread(iter, 1, iter->block_next);
	}

	free(iter);
	free(dir);

	return ptr;
}

// Return pointer to final entry in directory
unsigned dir_find_end(unsigned dir_ptr){
	Directory *dir = dir_load(dir_ptr);

	// Load first entry in dir
	Entry *iter = malloc(BLOCKSIZE);
	LBAread(iter, 1, dir->block_start);
	unsigned ptr = dir->block_start;

	// Continue loading dirs until end is reached
	while(iter->block_next != -1){
		ptr = iter->block_next;
		LBAread(iter, 1, iter->block_next);
	}

	// return ptr to final entry
	free(iter);
	free(dir);

	return ptr;
}

Directory *dir_load(unsigned dir_ptr){
	Directory *dir = malloc(BLOCKSIZE); 
	LBAread(dir, 1, dir_ptr);
	return dir;
}

Entry *entry_load(unsigned entry_ptr){
	Entry *entry = malloc(BLOCKSIZE); 
	LBAread(entry, 1, entry_ptr);
	return entry;
}

unsigned dir_resolve_path(char *path, unsigned dir){
	if(strlen(path) == 0){
		return dir;
	}

	if(path[0] == '/'){
		dir = g_super->ptr_root;
	}

	char *buffer = strdup(path);
	char *split;

	while((split = strsep(&buffer, "/")) != NULL) {
		// Root
		if(strlen(split) == 0){
			continue;
		}
		printf("Advance to dir %s\n", split);
		dir = dir_advance(split, dir);
		if(dir < 0){
			dir = -1;
			break;
		}
	}

	return dir;
}

void *file_read (char *name, unsigned dir_ptr){
    return 0;
}

int file_rm (char *name, unsigned dir_ptr){
    return 0;
}

int file_move (char *name, unsigned src, unsigned dest){
return 0;
}

int exfile_add (char *filepath, unsigned dir_ptr){
return 0;
}

int exfile_write (char *filepath, char *name, unsigned dir_ptr){
return 0;
}


// Change the current directory
int fs_change_dir (char *name){
	unsigned new_dir = dir_advance(name, g_cur_dir);

	if(new_dir == -1){
		printf("Cannot change directory into %s, not found", name);
		return -1;
	}

	if(new_dir == -2){
		printf("Cannot change directory into %s, not a directory", name);
		return -1;
	}

	g_cur_dir = new_dir;
    return 0;
}

unsigned dir_advance(char *name, unsigned dir_ptr){
	unsigned target_ptr = dir_find_entry(name, dir_ptr);

	if(target_ptr == -1){
		return -1;
	}

	Entry *target = malloc(BLOCKSIZE);
	LBAread(target, 1, target_ptr);
	unsigned result = target->block_data;

	if(target->is_dir == 0){
		free(target);
		return -2;
	}

	free(target);
	return result;
}

// Returns Directory object representing current dir (default: root)
unsigned fs_get_cur_dir (){
	return g_cur_dir;
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
	g_cur_dir = g_super->ptr_root;
    return 0;
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
}

