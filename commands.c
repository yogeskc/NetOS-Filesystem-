#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "low.h"
#include "utils.h"


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

/*char *freemap = NULL;

// Freemap functions
void freemap_init(){
	freemap = malloc(BLOCKCOUNT / sizeof(char));
}

void freemap_cleanup(){
	free(freemap);
}*/

/* Example freemap
freemap = {
	0x00011111,
	0x11111111,
	0x11000000,
	0x00000000
};

* Freemap after running freemap_set(0, 4, 16)

freemap = {
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000
}; */

void freemap_set(char *freemap, bool taken, unsigned blk_start, unsigned blk_end){
	// Calculate indexes within the freemap to modify
	int idx_start = blk_start / 8;
	int idx_end = blk_end / 8;

	// Calculate the offsets on the ends of the chars to skip
	int off_start = blk_start % 8;
	int off_end = blk_end % 8;

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
		*p = *p & m;
	}
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
