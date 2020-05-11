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

// Special version of dir_create, called when FS is created the first time
unsigned dir_create_root(){	
	// Allocate root directory and (..) entry
	Directory *root = malloc(sizeof(Directory));
	Entry *parent = malloc(sizeof(Entry));

	// Mark in freemap
	unsigned root_start = freemap_find_freespace(1, true);
	unsigned parent_start = freemap_find_freespace(1, true);

	if(root_start == -1 || parent_start == -1){
		printf ("Error: not enough space to create root dir!\n");
		fs_reload_freemap(); // undo freemap changes
		return -1;
	}

	// Fill out root data
	root->blk_start = parent_start;

	// Fill out (..) data
	strcpy(parent->name, "..");
	parent->blk_data = root_start;
	parent->blk_next = -1;
	parent->is_dir = 1;

	// Write to disk
	LBAwrite(root, 1, root_start);
	LBAwrite(parent, 1, parent_start);
	
	// free buffers
	free(root);
	free(parent);

	freemap_save();

	printf("Created dir root\n"); 
	printf("root dir @ %d\n", root_start);
	printf("root (..) @ %d\n", parent_start);

	return root_start;
}

// Create a new directory within a container dir
unsigned dir_create(char *name){
	// check if dir already exists
	if(dir_find_entry (name, g_cur_dir, false) != -1){
		printf("Entry %s already exists!\n", name);
		return -1;
	}

	// Check if dir name contains slashes
	if(strchr(name, '/') != NULL){
		printf("Cannot create: dir name %s contains a slash!\n", name);
		return -1;
	}

	if(strlen(name) > 256){
		printf("Cannot create dir: name > 256 chars!\n");
		return -1;
	}

	// Allocate directory and linker entries
	Entry *link = malloc(sizeof(Entry));
	Entry *parent = malloc(sizeof(Entry));
	Directory *dir = malloc(sizeof(Directory));

	// Find 3 blocks of free space, mark in freemap
	unsigned link_start = freemap_find_freespace(1, true);
	unsigned dir_start = freemap_find_freespace(1, true);
	unsigned parent_start = freemap_find_freespace(1, true);

	if(link_start == -1 || dir_start == -1 || parent_start == -1) {
		printf ("Error: not enough space to create the directories!\n");
		fs_reload_freemap(); // undo freemap changes
		return -1;
	}

	// Fill out link data
	// The link entry resides in the container, and points to the Directory block
	strcpy(link->name, name);
	link->blk_data = dir_start;
	link->blk_next = -1;
	link->is_dir = 1;

	// Fill out (..) data
	// The (..) entry resides in the new directory, and points back to the dir's container
	strcpy(parent->name, "..");
	parent->blk_data = g_cur_dir;
	parent->blk_next = -1;
	parent->is_dir = 1;
	
	// Fill out directory data
	// The Directory object holds the pointer to it's first entry, aka, the (..) entry
	dir->blk_start = parent_start;
	
	// Write to disk
	LBAwrite(link, 1, link_start);
	LBAwrite(parent, 1, parent_start);
	LBAwrite(dir, 1, dir_start);

	// free buffers
	free(link);
	free(parent);
	free(dir);

	// Append the link entry to the end of the container's entry chain
	if(entry_chain_append(g_cur_dir, link_start) == -1){
		printf("Failed to append new entry!\n");
		fs_reload_freemap(); // undo freemap changes
		return -1;
	}

	printf("Created dir %s\n", name); 
	printf("%s link @ %d\n", name, link_start);
	printf("%s dir @ %d\n", name, dir_start);
	printf("%s (..) @ %d\n", name, parent_start);

	freemap_save();

	return dir_start;
}

// Iterate across a directory's entry chain, print all names
int dir_list (unsigned dir_ptr, bool print_blocks){
	// Load directory associated with container ptr
	Directory *dir = malloc(BLOCKSIZE);
	LBAread(dir, 1, dir_ptr);

	printf("Listing DIR @ block %d\n", dir_ptr);

	// Allocate iterator
	Entry *iter = malloc(BLOCKSIZE);

	// Load first dir (should be ..)
	LBAread(iter, 1, dir->blk_start);

	// If blank name, something went wrong!!
	if(strlen(iter->name) == 0){
		printf("Error: directory @ block %d is correpted.\n", dir_ptr);
		return -1;
	}

	char *name = iter->name;

	// Print first name
	printf("%s", iter->name);
	if(iter->is_dir == 1)
		printf("/");
	if(print_blocks == true) 
		printf(" @ block %d", dir->blk_start);
	printf("\n");

	// Continue loading dirs until end is reached
	while(iter->blk_next != -1){
		unsigned tmp_block = iter->blk_next;
		LBAread(iter, 1, iter->blk_next);

		// Print first name
		printf("%s", iter->name);
		if(iter->is_dir == 1)
			printf("/");
		if(print_blocks == true) 
			printf(" @ block %d", tmp_block);
		printf("\n");
	}

	free(iter);
	free(dir);

	return 0;
}

// Recursive print directory structure
int dir_tree (unsigned dir_ptr, int level){
	// Load directory associated with container ptr
	Directory *dir = malloc(BLOCKSIZE);
	LBAread(dir, 1, dir_ptr);

	Entry *iter = malloc(BLOCKSIZE);

	// Load first dir & print it's name
	LBAread(iter, 1, dir->blk_start);

	// Continue loading dirs until end is reached
	while(iter->blk_next != -1){
		unsigned ptr = iter->blk_next;
		LBAread(iter, 1, iter->blk_next);

		if(strcmp(iter->name, "..") == 0){
			continue;
		}

		// Recursive print dir or just print entry name
		if(iter->is_dir == 1){
			for(int i = 0; i < level+1; i++)
				printf("|_");

			printf("%s/\n", iter->name);
			dir_tree(iter->blk_data, level+1);
		}else{
			for(int i = 0; i < level+1; i++)
				printf("|_");
			printf("%s\n", iter->name);
		}
	}

	free(iter);
	free(dir);

	return 0;
}

// Search an entry chain for a matching name, return block pointer
unsigned dir_find_entry (char *name, unsigned dir_ptr, bool before){
	Directory *dir = dir_load(dir_ptr);

	// Load first entry in dir
	Entry *iter = malloc(BLOCKSIZE);
	LBAread(iter, 1, dir->blk_start);
	unsigned ptr = dir->blk_start;
	unsigned ptr_last = -1;

	while(strcmp(iter->name, name) != 0){
		if(iter->blk_next == -1){
			ptr = -1;
			break;
		}

		// found match
		if(strcmp(iter->name, name) == 0){
			break;
		}

		ptr_last = ptr;
		ptr = iter->blk_next;
		LBAread(iter, 1, iter->blk_next);
	}

	free(iter);
	free(dir);

	if(before) return ptr_last;
	return ptr;
}

// Return pointer to final entry in a block chain
unsigned dir_find_end (unsigned dir_ptr){
	Directory *dir = dir_load(dir_ptr);

	// Load first entry in dir
	Entry *iter = malloc(BLOCKSIZE);
	LBAread(iter, 1, dir->blk_start);
	unsigned ptr = dir->blk_start;

	// Continue loading dirs until end is reached
	while(iter->blk_next != -1){
		ptr = iter->blk_next;
		LBAread(iter, 1, iter->blk_next);
	}

	// return ptr to final entry
	free(iter);
	free(dir);

	return ptr;
}

Directory *dir_load(unsigned block){
	Directory *dir = malloc(BLOCKSIZE); 
	LBAread(dir, 1, block);
	return dir;
}

Entry *entry_load(unsigned block){
	Entry *entry = malloc(BLOCKSIZE); 
	LBAread(entry, 1, block);
	return entry;
}

int entry_chain_append(unsigned dir_ptr, unsigned entry_ptr){
	unsigned dir_end_ptr = dir_find_end(dir_ptr);
	if(dir_end_ptr == -1){
		return -1;
	}

	Entry *dir_end = entry_load(dir_end_ptr);
	
	printf("Appending entry to %s\n", dir_end->name);

	dir_end->blk_next = entry_ptr;
	LBAwrite(dir_end, 1, dir_end_ptr);

	printf("%s block next = %d\n", dir_end->name, entry_ptr);

	freemap_save();

	free(dir_end);

	return 0;
}

unsigned resolve_path(char *path, unsigned dir){
	if(strlen(path) == 0){
		return dir;
	}

	if(path[0] == '/'){
		dir = g_super->blk_root;
		if(strlen(path) == 1) return dir;
	}

	char *buffer = strdup(path);
	char *split;
	Entry *search_entry = malloc(BLOCKSIZE);
	unsigned result = -1;

	// Iterate through slashes
	while((split = strsep(&buffer, "/")) != NULL) {
		if(strlen(split) == 0) continue;

		// split = next entry to search for 
		unsigned search_ptr = dir_find_entry(split, dir, false);

		// entry not found
		if(search_ptr == -1){
			return -1;
		}
		
		// Check if dir or file 
		LBAread(search_entry, 1, search_ptr);
		result = search_ptr;
		
		// If file, is end found?
		if(search_entry->is_dir == 0){
			free(buffer);
			free(search_entry);
			return search_ptr;
		}

		// If dir, advance and continue
		if(search_entry->is_dir == 1){
			dir = search_entry->blk_data;
		}
	}

	free(buffer);
	free(search_entry);

	return result;
}

// Remove a file from the filesystem. Update entry chains and freemap
int file_remove (char *name){
	unsigned rm_ptr = dir_find_entry(name, g_cur_dir, false);
	unsigned mod_ptr = dir_find_entry(name, g_cur_dir, true);

	if(rm_ptr == -1){
		printf("%s not found in current dir\n", name);
		return -1;
	}

	if(mod_ptr == -1){
		printf("prev entry not found in current dir\n");
		return -1;
	}


	Entry *rm = entry_load(rm_ptr);
	Entry *mod = entry_load(mod_ptr);

	if(rm->is_dir == 1){
		printf("Cannot remove %s, it's a directory!\n", rm->name);
		free(rm);
		free(mod);
		return -1;
	}

	printf("Remove entry %s @ %d, mod entry %s @ %d\n", rm->name, rm_ptr, mod->name, mod_ptr);

	unsigned rm_blocks = get_required_blocks(rm->size);

	// remove entry from container chain
	mod->blk_next = rm->blk_next;
	LBAwrite(mod, 1, mod_ptr);
	
	// mark entry and associated data as free
	printf("delete block %d\n", rm_ptr);
	printf("delete blocks %d->%d\n", rm->blk_data, rm->blk_data+rm_blocks);

	// update freemap
	freemap_set(0, 1, rm_ptr);
	freemap_set(0, rm_blocks, rm->blk_data);
	freemap_save();

	free(rm);
	free(mod);
	
    return 0;
}

// Move a file into another directory. Update entry chains and freemap
int file_move (char *name_src, char *name_dest){
	unsigned src_ptr = dir_find_entry(name_src, g_cur_dir, false);
	unsigned mod_ptr = dir_find_entry(name_src, g_cur_dir, true);
	unsigned dest_ptr = dir_find_entry(name_dest, g_cur_dir, false);

	if(src_ptr == -1){
		printf("%s not found in current dir\n", name_src);
		return -1;
	}

	if(mod_ptr == -1){
		printf("Error, no entry preceeding target entry\n");
		return -1;
	}

	if(dest_ptr == -1){
		printf("%s not found in current dir\n", name_dest);
		return -1;
	}

	Entry *src = entry_load(src_ptr);
	Entry *mod = entry_load(mod_ptr);
	Entry *dest = entry_load(dest_ptr);

	if(src->is_dir == 1){
		printf("Cannot move %s, it's not a file!\n", src->name);
		return -1;
	}

	if(dest->is_dir == 0){
		printf("Cannot move into %s, it's not a directory!\n", dest->name);
		return -1;
	}

	printf("Move entry %s @ %d, into dir %s @ %d\n", src->name, src_ptr, dest->name, dest_ptr);

	// remove entry from original chain
	mod->blk_next = src->blk_next;
	LBAwrite(mod, 1, mod_ptr);

	// append entry into dest chain 
	entry_chain_append(dest->blk_data, src_ptr);
	
	// update freemap
	freemap_save();
	
	return 0;
}

int file_copy (char *name_src, char *name_copy){
	unsigned copy_ptr = dir_find_entry(name_src, g_cur_dir, false);
	if(copy_ptr == -1){
		printf("File %s not found in cur directory\n", name_src);
		return -1;
	}

	// Check if file already exists
	if(dir_find_entry(name_copy, g_cur_dir, false) != -1){
		printf("File %s already exists!\n", name_copy);
		return -1;
	}


	Entry *copy = entry_load(copy_ptr);
	
	// Get entry data and size
	unsigned data_size = copy->size;
	unsigned data_blocks = get_required_blocks(data_size);
	void *data = malloc(BLOCKSIZE * data_blocks);
	LBAread(data, data_blocks, copy->blk_data);

	// Create a duplicate entry
	entry_create(name_copy, data, data_size);

	return 0;
	
}

// Modify an entry name in filesystem
int file_rename (char *name, char *new_name){
	unsigned target = dir_find_entry(name, g_cur_dir, false); //resolve_path(path, g_cur_dir);
	
	// invalid path
	if(target == -1){
		printf("Entry %s not found in cur directory\n", name);
		return -1;
	}

	// Check if renaming root??
	if(target == g_super->blk_root){
		printf("cannot rename root\n");
		return -1;
	}

	// Check if renaming a .. entry	
	if(strcmp(name, "..") == 0){
		printf("Cannot rename: new filename %s modifies a .. directory!\n", new_name);
		return -1;
	}

	// Check if new name contains a / character
	if(strchr(new_name, '/') != NULL){
		printf("Cannot rename: new filename %s contains a slash!\n", new_name);
		return -1;
	}

	// Copy over new name into entry, write to disk
	Entry *entry = entry_load(target);
	strcpy(entry->name, new_name);
	LBAwrite(entry, 1, target);
	freemap_save();

	return 0;
}

// Search for an external file (outside of NetFS) and copy it inside
int exfile_add(char *path_ext){

	void *data = get_file_data(path_ext);

	if(data == NULL){
		printf("Could not load file %s\n", path_ext);
		free(data);
		return -1;
	}

	unsigned data_size = get_file_size(path_ext);
	printf("data_size : %d\n", data_size);

	char *path_dup = strdup(path_ext);
	char *path_base = basename(path_dup);


	entry_create(path_base, data, data_size);

	free(path_dup);
	free(data);
	return 0;
}

int entry_create (char *name, void *data, unsigned data_size){
	if(dir_find_entry(name, g_cur_dir, false) != -1){
		printf("File %s already exists!\n", name);
		return -1;
	}

	// Find freespace for data
	unsigned data_blocks = get_required_blocks(data_size);
	printf("%d - required blocks\n", data_blocks);

	unsigned entry_ptr = freemap_find_freespace(1, true);
	unsigned data_ptr = freemap_find_freespace(data_blocks, true);
	if(data_ptr == -1){
		printf("Could not find %d free blocks!\n", data_blocks);
		fs_reload_freemap();
		return -1;
	}

	// Create new entry and point it to data
	Entry *entry = malloc(sizeof(Entry));
	strcpy(entry->name, name);
	entry->blk_data = data_ptr;
	entry->blk_next = -1;
	entry->size = data_size;
	entry->is_dir = 0;

	// append to entry chain
	entry_chain_append(g_cur_dir, entry_ptr);

	printf("Creating new entry %s, @ %d\n", entry->name, entry_ptr);
	printf("Creating data @ %d, %d blocks\n", data_ptr, data_blocks);

	// write to disk
	LBAwrite(data, data_blocks, data_ptr);
	LBAwrite(entry, 1, entry_ptr);

	freemap_save();

	free(entry);
	return 0;
}

// Search for an internal file, and copy it outside of NetFS filesystem
int exfile_write (char *name, char *path_ext){
	// resolve path to file entry
	unsigned entry_ptr = dir_find_entry(name, g_cur_dir, false); //resolve_path(path_int, g_cur_dir);

	if(entry_ptr == g_super->blk_root){
		printf("cannot write out root\n");
		return -1;
	}

	if(entry_ptr == -1){
		printf("Error: %s not found in cur dir\n", name);
		return -1;
	}

	Entry *entry = entry_load(entry_ptr);

	// Check if file
	if(entry->is_dir == 1){
		printf("Error: %s is a directory!\n", name);
		return -1;
	}

	// Load associated data
	unsigned req_blocks = get_required_blocks(entry->size);
	void *data = malloc(BLOCKSIZE * req_blocks);
	LBAread(data, req_blocks, entry->blk_data);
	printf("Data size %d, loading %d data blocks\n", entry->size, req_blocks);

	printf("%d\n", entry->size);

	// Write raw data to path_ext
	write_file_data(path_ext, data, entry->size);

	// free memory
	free(data);
	free(entry);

	return 0;
}

// Change the current directory
int fs_change_dir (char *name){
	unsigned target = dir_find_entry(name, g_cur_dir, false); //resolve_path(path, g_cur_dir);

	if(target == g_super->blk_root){
		g_cur_dir = g_super->blk_root;
		return 0;
	}

	if(target == -1){
		printf("Cannot change directory into %s, not found\n", name);
		return -1;
	}

	Entry *entry = entry_load(target);

	if(entry->is_dir == 0){
		printf("Cannot change directory into %s, not a directory", name);
		return -1;
	}

	g_cur_dir = entry->blk_data;
	free(entry);
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
	g_super->blk_freemap = freemap_get_start();
	g_super->len_freemap = freemap_get_len();
	g_super->blk_root = root_start;

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

	printf("Superblock loaded\n");

	// Allocate / Load freemap 
	freemap_load(g_super->blk_freemap, g_super->len_freemap);

	// Allocate / Load cur_dir
	g_cur_dir = g_super->blk_root;
    return 0;
}

void fs_reload_freemap(){
	freemap_load(g_super->blk_freemap, g_super->len_freemap);
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
		printf("Loading globals from existing filesystem\n");
		fs_load_globals();
	}

	// Load core structure (freemap & superblock)
	printf("Loaded core filesystem vars.\n");
	
	return part_status;
}

void fs_close(){
	closePartitionSystem();

	freemap_save();
	freemap_cleanup();
	free(g_super);
}

