#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "freemap.h"
#include "low.h"

char *g_freemap = NULL;
int freemap_start = 1;
int freemap_len = 0;
int freemap_size = 0;

void freemap_create(){
	g_freemap = malloc(FREEMAPSIZE);
	freemap_len = get_required_blocks(FREEMAPSIZE);
	freemap_set(1, freemap_len, freemap_start);
}

void freemap_load(unsigned ptr_freemap, unsigned len_freemap){
	g_freemap = malloc(FREEMAPSIZE);
	LBAread(g_freemap, len_freemap, ptr_freemap);
}

void freemap_save(){
	int freemap_len = get_required_blocks(FREEMAPSIZE);
	LBAwrite(g_freemap, freemap_len, freemap_start);
}

void freemap_cleanup(){
	if(g_freemap == NULL) return;
	free(g_freemap);
	g_freemap = NULL;
}

// Freemap getters

int freemap_get_start(){
	return freemap_start;
}

int freemap_get_len(){
	return freemap_len;
}

// Freemap modification / reading functions

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


