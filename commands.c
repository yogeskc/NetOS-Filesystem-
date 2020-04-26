#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "low.h"
#include "utils.h"

// Global fs vars
char *g_freemap = NULL;
Superblock *g_super = NULL;

// Given a filesize in bytes, calcualte it's required blocks to be stored
unsigned get_required_blocks(unsigned size){
	unsigned block_count = round(size / BLOCKSIZE);
	if(block_count == 0) block_count = 1;
	return block_count;
}

// take a file from outside the NetOS filesystem and copy it in
int fs_add_file(char *filepath, unsigned data_start){
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

void freemap_set(bool taken, unsigned blk_len, unsigned blk_start){
	if(g_freemap == NULL){
		printf("Error: Freemap has not been initialized yet!\n");
		return;
	}

	unsigned blk_end = blk_start + blk_len;

	// Clamp values
	if(blk_start < 0){
		blk_start = 0;
	}
	if(blk_end > FREEMAPSIZE){
		blk_end = FREEMAPSIZE;
	}

	// Calculate indexes within the freemap to modify
	int idx_start = blk_start / 8;
	int idx_end = blk_end / 8;

	// Calculate the offsets on the ends of the chars to skip
	int off_start = blk_start % 8;
	int off_end = (blk_end % 8);

	// printf("Setting bits on freemap idx %d+%d -> %d+%d\n", idx_start, off_start, idx_end, off_end);

	// This for loop will read every single content from blk_start to blk_end
	for(int i = idx_start; i <= idx_end; i++){
		// pointer to current 8 blocks in freemap
		char *p = &g_freemap[i];

		// new array for copying the content of a freemap char
		int mask[8] = {0,0,0,0,0,0,0,0};

		// If block contains other values which we don't want to modify,
		// skip over them using mask_start and mask_end. Helps select the right
		// blocks to modify.
		int mask_start = 0;
		int mask_end = 8;

		// set the mask_start and end
		if(i == idx_start){
			mask_start = off_start;
		}

		if(i == idx_end){
			mask_end = off_end;
		}

		// Generate mask
		for(int j = 0; j < 8; j++){
			// Skip over values NOT within the mask_start -> mask_end range
			if(j >= mask_start && j <= mask_end){
				mask[j] = (int)taken;
				continue;
			}

			// Maintain old values from original char
			mask[j] = p[j];
		}

		// Convert into binary
		char m = bits2byte(mask);

		//Apply mask
		// Or mask for shifting 0's to 1's
		if((int) taken == 1)
			*p = *p | m;

		// And mask for 0-ing out 1's
		if((int) taken == 0)
			*p = *p & m;
	}
}

// Find the first contiguous free space with atleast 'blk_len' blocks available.
// If no free spaces are availabe, then return -1
// If auto_modify is set to true, will automatically mark the free blocks as taken
unsigned freemap_find_freespace(unsigned blk_len){
	if(g_freemap == NULL){
		printf("Error: Freemap has not been initialized yet!\n");
		return -1;
	}

	int counter = 0; // count of contiguous free spaces
	int free_start_idx = 0; // start index (in freemap) of the current free space chain
	int free_start_off = 0; // start offset (in freemap) of the current free space chain

	// Search entire freemap
	for(int i = 0; i < FREEMAPSIZE; i++){
		int *bits = byte2bits(g_freemap[i]);
		//printbyte(g_freemap[i]);

		// Search each row of freemap
		for(int j = 0; j < 8; j++){
			// If a 1 is encountered, the free space is NOT contiguous. Reset counter
			if(bits[j] == 1){
				counter = 0;
				continue;
			}

			// If the counter was just reset, set the current index to the free_start_idx.
			if(counter == 0){
				free_start_idx = i;
				free_start_off = j;
			}
			counter += 1;

			// Found 'blk_len' amount of contiguous free spaces! success
			if(counter >= blk_len){
				free(bits);
				return (free_start_idx*8) + j - 1;
			}
		}

		free(bits);
	}

	// Couldn't find enough contiguous free spaces
	return -1;
}

// Directory functions
unsigned fs_create_root_dir(){
	// Create root dir
	Directory *root = malloc(sizeof(Directory));
	
	// Find space for root dir and (..) entry
	unsigned root_start = freemap_find_freespace(1);
	freemap_set(1, 1, root_start);
	unsigned parent_start = freemap_find_freespace(1);
	freemap_set(1, 1, parent_start);

	// Point root towards the (..) entry
	root->block_start = parent_start;

	// Create (..) entry
	Entry *parent = malloc(sizeof(Entry));
	strcpy(parent->name, "..");
	parent->block_data = root_start; 		// Point the (..) entry back towards the root dir
	parent->block_next = -1;

	// Write to disk
	LBAwrite(root, 1, root_start);
	LBAwrite(parent, 1, parent_start);

	free(root);
	free(parent);

	return root_start;
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
	Entry *iter = malloc(BLOCKSIZE);

	// Load first dir & print it's name
	LBAread(iter, 1, dir->block_start);
	printf("%s\n", iter->name);

	// Continue loading dirs until end is reached
	while(iter->block_next != -1){
		printf("%s\n", iter->name);
		LBAread(iter, 1, iter->block_next);
	}

	return 0;
}

// Initial filesystem creation
int fs_init(){
	printf("Initialize core system\n");
	
	// Allocate and mark freemap
	g_freemap = malloc(FREEMAPSIZE);
	int freemap_start = 1;
	int freemap_len = get_required_blocks(FREEMAPSIZE);
	freemap_set(1, freemap_len, freemap_start);

	// Allocate and mark superblock
	g_super = malloc(sizeof(Superblock));
	freemap_set(1, 1, 0);

	// Write root dir to disk (after freemap)
	unsigned root_start = fs_create_root_dir();
	printf("Wrote root dir at block %d\n", root_start);

	// Create superblock (block 0)
	g_super->ptr_freemap = freemap_start;
	g_super->len_freemap = freemap_len;
	g_super->ptr_root = root_start;

	// Write core system to disk
	printf("Wrote superblock\n");
	LBAwrite(g_super, 1, 0);

	printf("Wrote %d empty freemap blocks\n", freemap_len);
	LBAwrite(g_freemap, freemap_len, freemap_start);

	return 0;
}

int fs_load_globals(){
	// Allocate / Load superblock
	g_super = malloc(BLOCKSIZE);
	LBAread(g_super, 1, 0);

	// Allocate / Load freemap 
	g_freemap = malloc(FREEMAPSIZE);
	LBAread(g_freemap, g_super->len_freemap, g_super->ptr_freemap);
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

	free(g_freemap);
	free(g_super);
}
