#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <libgen.h>

#include "commands.h"
#include "freemap.h"
#include "low.h"
#include "utils.h"

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
	if(dir_find_entry (name, container_ptr, false) != -1){
		printf("Entry %s already exists!\n", name);
		return -1;
	}

	if(strchr(name, '/') != NULL){
		printf("Cannot create: dir name %s contains a slash!\n", name);
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
	/*unsigned cnt_end_ptr = dir_find_end(container_ptr);
	Entry *cnt_end = malloc(BLOCKSIZE);

	LBAread(cnt_end, 1, cnt_end_ptr);
	cnt_end->block_next = link_start;
	LBAwrite(cnt_end, 1, cnt_end_ptr);

	free(cnt_end);*/

	entry_chain_append(container_ptr, link_start);

	// Update freemap
	freemap_save();

	return dir_start;
}

unsigned dir_move(char *path_src, char *path_dest, unsigned container_ptr){
	// find directory containing entry
	// find entry itself
	// remove entry from container chain
	// append entry to destination chain
	
	return -1;
}

int dir_rm(char *path_src, unsigned container_ptr){
	// find directory containing entry 
	// find entry itself
	// remove entry from container chain
	// recursive remove files and directories within
	// mark as free
	
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

int dir_tree(unsigned dir_ptr, int level){
	// Load directory associated with container ptr
	Directory *dir = malloc(BLOCKSIZE);
	LBAread(dir, 1, dir_ptr);

	for(int i = 0; i < level; i++)
		printf("|_");

	printf("%s/\n", dir->name, dir_ptr);

	Entry *iter = malloc(BLOCKSIZE);

	// Load first dir & print it's name
	LBAread(iter, 1, dir->block_start);

	// Continue loading dirs until end is reached
	while(iter->block_next != -1){
		unsigned ptr = iter->block_next;
		LBAread(iter, 1, iter->block_next);

		if(strcmp(iter->name, "..") == 0){
			continue;
		}

		// Recursive print dir or just print entry name
		if(iter->is_dir == 1){
			dir_tree(iter->block_data, level+1);
		}else{
			for(int i = 0; i < level+1; i++)
				printf("|_");
			printf("%s\n", iter->name, iter->block_data);
		}
	}

	free(iter);
	free(dir);

	return 0;
}

unsigned dir_find_entry (char *name, unsigned dir_ptr, bool before){
	Directory *dir = dir_load(dir_ptr);

	// Load first entry in dir
	Entry *iter = malloc(BLOCKSIZE);
	LBAread(iter, 1, dir->block_start);
	unsigned ptr = dir->block_start;
	unsigned ptr_last = -1;

	while(strcmp(iter->name, name) != 0){
		if(iter->block_next == -1){
			ptr = -1;
			break;
		}

		// found match
		if(strcmp(iter->name, name) == 0){
			break;
		}

		ptr_last = ptr;
		ptr = iter->block_next;
		LBAread(iter, 1, iter->block_next);
	}

	free(iter);
	free(dir);

	if(before) return ptr_last;
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

void *entry_load_data(Entry *e){
	unsigned req_blocks = get_required_blocks(e->size);
	void *data = malloc(BLOCKSIZE * req_blocks);
	LBAread(data, req_blocks, e->block_data);
	printf("Data size %d, loading %d data blocks\n", e->size, req_blocks);
	return data;
}

int entry_chain_append(unsigned container_ptr, unsigned entry_ptr){
	unsigned dir_end_ptr = dir_find_end(container_ptr);
	if(dir_end_ptr == -1){
		return -1;
	}

	Entry *dir_end = entry_load(dir_end_ptr);
	
	printf("Appending entry to %s\n", dir_end->name);

	dir_end->block_next = entry_ptr;
	LBAwrite(dir_end, 1, dir_end_ptr);

	printf("%s block next = %d\n", dir_end->name, entry_ptr);

	freemap_save();

	free(dir_end);

	return 0;
}

int entry_chain_remove(char *name, unsigned container_ptr){
	unsigned mod_ptr = dir_find_entry(name, container_ptr, true);
	unsigned rm_ptr = dir_find_entry(name, container_ptr, false);

	if(mod_ptr == -1 || rm_ptr == -1) return -1;

	Entry *mod = entry_load(mod_ptr);
	Entry *rm = entry_load(rm_ptr);

	mod->block_next = rm->block_next;
	LBAwrite(mod, 1, mod_ptr);

	freemap_save();

	free(mod);
	free(rm);

	return 0;
}

unsigned resolve_path(char *path, unsigned dir, bool before){
	if(strlen(path) == 0){
		return dir;
	}

	if(path[0] == '/'){
		dir = g_super->ptr_root;
	}

	char *buffer = strdup(path);
	char *split;
	unsigned last_dir = dir;

	while((split = strsep(&buffer, "/")) != NULL) {
		// Root
		if(strlen(split) == 0){
			continue;
		}
		printf("Advance to dir %s\n", split);
		last_dir = dir;
		// dir = dir_advance(split, dir);
		
		// dir advance
		unsigned target_ptr = dir_find_entry(split, dir, false);
		if(target_ptr == -1){
			return -1;
		}

		Entry *target = malloc(BLOCKSIZE);
		LBAread(target, 1, target_ptr);
		unsigned result = target->block_data;

		// found the matching entry, cannot advance anymore
		if(target->is_dir == 0){
			dir = target_ptr;
			free(target);
			break;
		}

		free(target);
		dir = result;

		if(dir < 0){
			dir = -1;
			break;
		}
	}

	if(before == true) return last_dir;
	return dir;
}

void *file_read (char *path, unsigned container_ptr){
	// resolve path to entry
	// return data associated with file
    return 0;
}

int file_rm (char *path, unsigned container_ptr){
	// resolve path to entry
	// resolve path to container dir 
	// remove entry from container chain
	// mark entry and associated data as free
	
    return 0;
}

int file_move (char *name, unsigned src, unsigned dest){
	// resolve path to entry
	// resolve path to container dir 
	// remove entry from container chain
	// append entry to dest chain
	
	return 0;
}

int file_rename (char *path, char *new_name, unsigned container_ptr){
	unsigned target = resolve_path(path, container_ptr, false);
	if(target == -1){
		printf("Entry %s not found!\n", path);
		return -1;
	}
	if(strchr(new_name, '/') != NULL){
		printf("Cannot rename: new filename %s contains a slash!\n", new_name);
		return -1;
	}

	// Check if new name has collissions with anything in the dir
	/*unsigned new_container = resolve_path(path, container_ptr, true);
	if(dir_find_entry (new_name, new_container, false) != -1){
		printf("Entry %s already exists!\n", name);
		return -1;
	}*/

	Entry *entry = entry_load(target);
	strcpy(entry->name, new_name);
	LBAwrite(entry, 1, target);

	freemap_save();
	return 0;
}

int exfile_add (char *path_ext, unsigned container_ptr){
	void *data = get_file_data(path_ext);

	if(data == NULL){
		printf("Could not load file %s\n", path_ext);
		return -1;
	}

	unsigned data_size = get_file_size(path_ext);
	printf("data_size : %d\n", data_size);

	// Find freespace for data
	unsigned data_blocks = get_required_blocks(data_size);
	printf("%d - required blocks\n", data_blocks);
	unsigned data_ptr = freemap_find_freespace(data_blocks+1, true);
	if(data_ptr == -1){
		printf("Could not find %d free blocks!\n", data_blocks);
		return -1;
	}

	// shift data ptr one over, the first block is for the entry
	data_ptr += 1;
	unsigned entry_ptr = data_ptr-1;

	// Create new entry and point it to data
	Entry *entry = malloc(sizeof(Entry));
	strcpy(entry->name, basename(path_ext));
	entry->block_data = data_ptr;
	entry->block_next = -1;
	entry->size = data_size;
	entry->is_dir = 0;

	// append to entry chain
	/*unsigned cnt_end_ptr = dir_find_end(container_ptr);
	Entry *cnt_end = malloc(BLOCKSIZE);

	LBAread(cnt_end, 1, cnt_end_ptr);
	cnt_end->block_next = entry_ptr;
	LBAwrite(cnt_end, 1, cnt_end_ptr;
	free(cnt_end);*/

	// append to entry chain
	entry_chain_append(container_ptr, entry_ptr);

	printf("Creating new entry %s:%d\n", entry->name, entry_ptr);
	printf("Creating data %d->%d\n", data_ptr, data_blocks);

	// write to disk
	LBAwrite(data, data_blocks, data_ptr);
	LBAwrite(entry, 1, entry_ptr);

	free(entry);
	free(data);

	freemap_save();
	
	return 0;
}


int exfile_write (char *path_int, char *path_ext, unsigned container_ptr){
	// resolve path to file entry
	unsigned entry_ptr = resolve_path(path_int, container_ptr, false);

	if(entry_ptr == -1){
		printf("Error: %s does not exist\n", path_int);
		return -1;
	}

	Entry *entry = entry_load(entry_ptr);
	printf("Resolved to entry %s:%d\n", entry->name, entry_ptr);

	// Check if file
	if(entry->is_dir == 1){
		printf("Error: %s is a directory!\n", path_int);
		return -1;
	}

	// Load associated data
	void *data = entry_load_data(entry);

	printf("%d\n", entry->size);

	// Write raw data to path_ext
	write_file_data(path_ext, data, entry->size);

	// free memory
	free(data);
	free(entry);

	return 0;
}

// Change the current directory
int fs_change_dir (char *path){
	unsigned new_dir = resolve_path(path, g_cur_dir, false);

	if(new_dir == -1){
		printf("Cannot change directory into %s, not found", path);
		return -1;
	}

	if(new_dir == -2){
		printf("Cannot change directory into %s, not a directory", path);
		return -1;
	}

	g_cur_dir = new_dir;
    return 0;
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

