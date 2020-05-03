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

// take a file from outside the NetOS filesystem and copy it into current dir
int fs_add_file_external(char *filepath){
	/*void *buffer = get_file_data(filepath);
	int buf_size = get_file_size(filepath);

	if(buffer == NULL){
		printf("Error: %s file doesn’t exist!\n", filepath);
		return -1;
	}

	// Create new entry
	Entry *new_entry = (Entry *) malloc(sizeof(Entry));
	strcpy(new_entry->name, filepath);
	new_entry->name[sizeof(new_entry->name)-1] = '\0';

	// Detect record length and allocate blocks
	new_entry->size = buf_size;

	printf("Allocating %d blocks in filesystem\n", get_required_blocks(buf_size));

	// create nuggets
	Nugget nug_cur;
	Nugget nug_next;
	nug_cur.block_data = data_start+1;
	nug_next.block_data = -1;

	// set nuggets
	new_entry->nug_cur = nug_cur;
	new_entry->nug_next = nug_next;

	LBAwrite( buffer, get_required_blocks(buf_size), nug_cur.block_data);  //stores raw data from file
	LBAwrite( (void *)new_entry, 1, data_start); // putting an entry

	free(buffer);
	free(new_entry);*/

	return 0;
}

int fs_write_file_external(char *name){
	return 0;
}

// Load the directory entry at data_start
void *fs_read_file(unsigned data_start){
	/*void *buffer = malloc(BLOCKSIZE);

	// Load the directory entry @ data_start
	LBAread(buffer, 1, data_start);

	// Convert raw data into Entry
	Entry *entry = (Entry *)buffer;

	// Allocate space for the data which the Entry points to
	void *entry_data = malloc(entry->size);

	// Load the data associated with the Entry
	LBAread( entry_data, get_required_blocks(entry->size), entry->nug_cur.block_data);

	free(buffer);
	return entry_data;*/
	return NULL;
}

// Remove a file entry based on it's container dir and name
int file_rm (Directory *dir, char *name){
	return 0;
}

// Move a file with matching 'name', within dir 'src', into dir 'dest'
int file_move (Directory *src, Directory *dest, char *name){
	return 0;
}


// Add a new Entry into a directory chain, sync with LBAwrite
unsigned dir_entry_append(unsigned new_ent_ptr, Entry *new_ent, Directory *dir){
	Entry *iter = malloc(BLOCKSIZE);
	unsigned iter_ptr = dir->block_start;

	// Load first dir & check if it's the end
	LBAread(iter, 1, iter_ptr);

	// Continue loading dirs until end is reached
	while(iter->block_next != -1){
		iter_ptr = iter->block_next;
		LBAread(iter, 1, iter_ptr);
	}

	// modify the end block, and point it to the new dir
	iter->block_next = new_ent_ptr;
	new_ent->block_next = -1;

	// write changes to disk
	LBAwrite(iter, 1, iter_ptr);
	LBAwrite(new_ent, 1, new_ent_ptr);

	// update freemap
	freemap_set(1, 1, iter_ptr);
	freemap_set(1, 1, new_ent_ptr);
	freemap_save();

	free(iter);

	return 0;
}

unsigned dir_rm(char *name, Directory *dir){
	Entry *iter = malloc(BLOCKSIZE);
	unsigned iter_ptr = dir->block_start;

	// Load first dir & check if it's the end
	LBAread(iter, 1, iter_ptr);

	// Continue loading dirs until end is reached
	while(iter->name != name){
		iter_ptr = iter->block_next;
		LBAread(iter, 1, iter_ptr);

		// Target entry not found
		if(iter->block_next == -1){
			return -1;
		}
	}

}

// Remove data associated with an entry
unsigned dir_entry_rm_data(Entry *ent){
	if(ent->is_dir == true){
		//dir_rm(ent->block_data);
	}else{
		freemap_set(0, get_required_blocks(ent->size), ent->block_data);
	}
}

// Remove an Entry from a directory chain, sync with LBAwrite
unsigned dir_entry_rm(char *name, Directory *dir){
	Entry *iter = malloc(BLOCKSIZE);
	unsigned iter_ptr = dir->block_start;

	// Load first dir & check if it's the end
	LBAread(iter, 1, iter_ptr);

	// Continue loading dirs until end is reached
	while(iter->name != name){
		iter_ptr = iter->block_next;
		LBAread(iter, 1, iter_ptr);

		// Target entry not found
		if(iter->block_next == -1){
			return -1;
		}
	}

	// Load the dir to remove
	unsigned rm_block = iter->block_next;
	Entry *to_remove = malloc(BLOCKSIZE);
	LBAread(to_remove, 1, rm_block);

	// Remove entry from chain
	iter->block_next = to_remove->block_next;

	// Write changes
	LBAwrite(iter, 1, iter_ptr);

	// update freemap
	freemap_set(0, 1, rm_block);
	freemap_save();

	free(iter);
	free(to_remove);

	return 0;
}

unsigned dir_create(char *name, Directory *container, bool is_root){
	// Entry within container dir (link point to our new dir)
	Entry *link = NULL;
	unsigned link_start = -1;

	// check if dir already exists
	if(is_root == false){
		Entry *search = dir_find_entry (name, container);
		if(search != NULL){
			printf("Directory %s already exists!\n", name);
			free(search);
			return -1;
		}
	}

	// Find space / allocate link entry
	if(is_root == false){
		link = malloc(sizeof(Entry));
		link_start = freemap_find_freespace(1);
		freemap_set(1,1, link_start);

		strcpy(link->name, name);
		link->is_dir = 1;
	}
	
	// Find space / allocate directory 
	Directory *dir = malloc(sizeof(Directory));

	unsigned dir_start = freemap_find_freespace(1);
	freemap_set(1, 1, dir_start);

	// The link entry will point to our new directory
	if(is_root == false){
		link->block_data = dir_start;
	}

	// Find space / allocate (..) entry
	Entry *parent = malloc(sizeof(Entry));
	unsigned parent_start = freemap_find_freespace(1);
	freemap_set(1, 1, parent_start);

	// (..) entry is the first thing in our new directory.
	dir->block_start = parent_start;
	dir->block_dir = dir_start;
	strcpy(dir->name, name);
	strcpy(parent->name, "..");
	parent->is_dir = 1;

	// Point (..) entry back to it's container (directory above)
	// (..) entry for root points back to root
	// Otherwise, (..) entry points back to container
	if(is_root == true){
		parent->block_data = dir_start;
	}else{
		parent->block_data = container->block_dir;
		printf("Container starts at %d\n", container->block_start);
	}

	// (..) entry is the only one, so set next ptr to = -1 (end of linkedlist)
	parent->block_next = -1;

	// Write to disk
	LBAwrite(dir, 1, dir_start);
	LBAwrite(parent, 1, parent_start);
	
	// Write link entry to container dir (append at end of linkedlist)
	if(is_root == false){
		dir_entry_append(link_start, link, container);
		free(link);
	}	

	// Update freemap
	freemap_save();

	// free buffers
	free(dir);
	free(parent);

	return dir_start;
}

Directory *dir_load(unsigned blk_start){
	Directory *dir = malloc(BLOCKSIZE);

	if(LBAread(dir, 1, blk_start) == -1){
		printf("Error reading directory\n");
		free(dir);
		return NULL;
	}

	return dir;
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
Entry *dir_find_entry (char *name, Directory *dir){
	Entry *iter = malloc(BLOCKSIZE);

	// Load first dir & print it's name
	LBAread(iter, 1, dir->block_start);

	// Check if names match
	if(strcmp(iter->name, name) == 0){
		return iter;
	}

	// Continue loading dirs until end is reached
	while(iter->block_next != -1){
		LBAread(iter, 1, iter->block_next);

		// Check if names match
		if(strcmp(iter->name, name) == 0){
			return iter;
		}
	}

	// Dir not found...
	free(iter);
	return NULL;
}

// Initial filesystem creation
int fs_init(){
	printf("Initialize core system\n");
	
	// Create a blank freemap, then mark it.
	freemap_create();

	// Allocate and mark superblock
	g_super = malloc(sizeof(Superblock));
	freemap_set(1, 1, 0);

	// Write root dir to disk (after freemap)
	unsigned root_start = dir_create("root", NULL, true);
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
		part_status = fs_init();
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
	Entry *target = dir_find_entry(name, g_cur_dir);

	if(target == NULL){
		printf("Cannot cd to %s, directory not found\n", name);
		return -1;
	}

	if(target->is_dir == 0){
		printf("Cannot cd to %s, target is not a directory\n", target->name);
		return -1;
	}

	printf("Loading dir at %d\n", target->block_data);

	LBAread(g_cur_dir, 1, target->block_data);
}

// Returns Directory object representing current dir (default: root)
Directory *fs_get_cur_dir (){
	return g_cur_dir;
}
