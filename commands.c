#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "low.h"
#include "utils.h"

// Freemap variable
char *freemap = NULL;

// Given a filesize in bytes, calcualte it's required blocks to be stored
unsigned get_required_blocks(unsigned size){
	unsigned block_count = round(size / BLOCKSIZE);
	if(block_count == 0) block_count = 1;
	return block_count;
}

// take a file from outside the NetOS filesystem and copy it in
int fs_add_file(char *filepath, unsigned data_start){
	void *buffer = get_file_data(filepath);
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
	nug_cur.block_start = data_start+1;
	nug_next.block_start = -1;

	// set nuggets
	new_entry->nug_cur = nug_cur;
	new_entry->nug_next = nug_next;

	LBAwrite( buffer, get_required_blocks(buf_size), nug_cur.block_start);  //stores raw data from file
	LBAwrite( (void *)new_entry, 1, data_start); // putting an entry

	free(buffer);
	free(new_entry);

	return 0;
}

// Load the directory entry at data_start
void *fs_read_file(unsigned data_start){
	void *buffer = malloc(BLOCKSIZE);

	// Load the directory entry @ data_start
	LBAread(buffer, 1, data_start);

	// Convert raw data into Entry
	Entry *entry = (Entry *)buffer;

	// Allocate space for the data which the Entry points to
	void *entry_data = malloc(entry->size);

	// Load the data associated with the Entry
	LBAread( entry_data, get_required_blocks(entry->size), entry->nug_cur.block_start);

	free(buffer);
	return entry_data;
}

// Freemap funcs
void freemap_init(){
	freemap = malloc(FREEMAPSIZE);
	printf("Allocating freemap size %lu\n", FREEMAPSIZE);
}

//cleaning up free map
void freemap_cleanup(){
	if(freemap == NULL){
		printf("Error: Freemap has not been initialized yet!\n");
		return;
	}
  
	free(freemap);
	freemap = NULL;
}

// getting the free-starting space to the ending space

void freemap_set(bool taken, unsigned blk_start, unsigned blk_end){
	if(freemap == NULL){
		printf("Error: Freemap has not been initialized yet!\n");
		return;
	}

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

	printf("Setting bits on freemap idx %d+%d -> %d+%d\n", idx_start, off_start, idx_end, off_end);

	// This for loop will read every single content from blk_start to blk_end
	for(int i = idx_start; i <= idx_end; i++){
		// pointer to current 8 blocks in freemap
		char *p = &freemap[i];

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
	if(freemap == NULL){
		printf("Error: Freemap has not been initialized yet!\n");
		return -1;
	}

	int counter = 0; // count of contiguous free spaces
	int free_start_idx = 0; // start index (in freemap) of the current free space chain
	int free_start_off = 0; // start offset (in freemap) of the current free space chain

	// Search entire freemap
	for(int i = 0; i < FREEMAPSIZE; i++){
		int *bits = byte2bits(freemap[i]);
		printbyte(freemap[i]);

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
				return (free_start_idx*8) + j - 1;
			}
		}
	}

	// Couldn't find enough contiguous free spaces
	return -1;
}

// Directory functions
unsigned fs_create_root_dir(){
	// Create root dir
	Directory *root = malloc(sizeof(Directory));
	unsigned root_start = freemap_find_freespace(1);		// Find space for root dir
	freemap_set(1, root_start, root_start);  //writing the root dir into it

	// Find space for (..) entry, point root dir towards it
	root->block_start = freemap_find_freespace(1);
	freemap_set(1, root->block_start, root->block_start);

	// Create (..) entry
	Entry *parent = malloc(sizeof(Entry));
	Nugget parent_nug;
	Nugget dummy_nug;

	parent_nug.block_start = root_start; 		// Point the (..) entry back towards the root dir
	dummy_nug.block_start = -1;

	// Fill in (..) entry data
	strcpy(parent->name, "..");
	parent->nug_cur = parent_nug;
	parent->nug_next = dummy_nug;

	LBAwrite( (void *) root, root_start, 1);
	LBAwrite( (void *) parent, root->block_start, 1);

	return root_start;
}

void fs_create_dir(){
	//
}

// Initial filesystem creation
int fs_init(){
	printf("Creating superblock, freemap and root dir\n");

	// Allocate and mark freemap
	freemap_init();
	int freemap_start = 1;
	int freemap_len = get_required_blocks(FREEMAPSIZE);
	freemap_set(1, freemap_start, freemap_start+freemap_len);

	// Allocate and mark superblock
	Superblock *super = malloc(sizeof(Superblock));
	freemap_set(1, 0, 1);

	// Write freemap to disk (block 1)
	LBAwrite( freemap, freemap_start, freemap_len);
	printf("Wrote %d empty freemap blocks\n", freemap_len);

	// Write root dir to disk (after freemap)
	unsigned root_start = fs_create_root_dir();
	printf("Created root dir at block %d\n", root_start);

	// Create superblock (block 0)
	super->ptr_freemap = freemap_start;
	super->len_freemap = freemap_len;
	super->ptr_root = root_start;

	printf("Wrote superblock\n");

	// Write superblock to disk
	LBAwrite((void *)super, 0, 1);

	return 0;
}

// Locate 'filename' and start filesystem off of it. Initialize the filesystem if necessary.
int fs_start(char *filename){
	// Create filesystem file
	uint64_t blockSize = BLOCKSIZE;
	uint64_t volumeSize = VOLSIZE;

	int part_status = startPartitionSystem(filename, &volumeSize, &blockSize);
	printf("Partition system started with status %d\n", part_status);

	//
	switch (part_status){
		default:
			printf("Error creating the filesystem file!\n");
		break;

		case 0:
			printf("Filesystem already created. Loading.\n");
		break;

		case 2:
			//part_status = fs_init();
		break;
	}

	part_status = fs_init();

	return part_status;
}

void fs_close(){
	freemap_cleanup();
	closePartitionSystem();
}

/*
void fs_create_root(){
// Place directly after superblock
int root_block_start = 2;

// Create the .. entry
Entry *root_parent_entry = (Entry *) malloc(sizeof(Entry));
strcpy(root_parent_entry->name, "..");
root_parent_entry->size = root_block_start;

// Root .. entry
Nugget root_parent_nugget;
root_parent_nugget.block_start = 2;
root_parent_entry->nug_cur = root_parent_nugget;
root_parent_entry->nug_next = NULL;

free(root_parent_entry);
}

void fs_create_dir(char *name, unsigned parent, unsigned block){
// Create '..' directory, which points to parent
Entry *new_entry = (Entry *) malloc(sizeof(Entry));
strcpy(new_entry->name, "..");
new_entry->size = 0;
}*/
