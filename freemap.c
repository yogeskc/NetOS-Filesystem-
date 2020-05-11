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
	freemap_start = ptr_freemap;
	freemap_len = len_freemap;
	LBAread(g_freemap, len_freemap, ptr_freemap);
}

void freemap_save(){
	int freemap_len = get_required_blocks(FREEMAPSIZE);
	LBAwrite(g_freemap, freemap_len, freemap_start);
}

void freemap_cleanup(){
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

int freemap_set(int taken, unsigned blk_len, unsigned blk_start){
	if(g_freemap == NULL){
		printf("Error: Freemap has not been initialized yet!\n");
		return -1;
	}

	unsigned blk_end = blk_start + blk_len - 1;

	// Clamp values
	if(blk_start < 0){
		blk_start = 0;
	}
	if(blk_end > BLOCKCOUNT){
		printf("out of range!\n");
		return -1;
	}

	// Calculate indexes within the freemap to modify
	int idx_start = blk_start / 8;
	int idx_end = blk_end / 8;

	// Calculate the offsets on the ends of the chars to skip
	int off_start = (blk_start % 8);
	int off_end = (blk_end % 8);

	printf("Setting bits on freemap idx %d+%d -> %d+%d\n", idx_start, off_start, idx_end, off_end);

	// This for loop will read every single content from blk_start to blk_end
	for(int i = idx_start; i <= idx_end; i++){
		// pointer to current 8 blocks in freemap
		char *p = &g_freemap[i];
		int *pmask = byte2bits(*p);

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
			//printf("clipping to start @ %d\n", idx_start);
		}

		if(i == idx_end){
			mask_end = off_end;
			//printf("clipping to end @ %d\n", idx_end);
		}

		// Generate mask
		for(int j = 0; j < 8; j++){
			// Skip over values NOT within the mask_start -> mask_end range
			if(j >= mask_start && j <= mask_end){
				mask[j] = taken;
				continue;
			}

			// Maintain old values from original char
			mask[j] = pmask[j];
			//printf("maintain %d:%d = %d\n", i, j, pmask[j]);
		}

		// Convert into binary
		char m = bits2byte(mask);

		/*printbyte(m);
		if(taken == 1)
			printf("||||||||");
		else
			printf("&&&&&&&&");

		printf("\n");
		printbyte(*p);*/

		//Apply mask
		// Or mask for shifting 0's to 1's
		if(taken == 1)
			*p = *p | m;

		// And mask for 0-ing out 1's
		if(taken == 0)
			*p = *p & m;

		/*printbyte(*p);
		printf("\n");*/

		free(pmask);

	}

	return 0;
}

// Find the first contiguous free space with atleast 'blk_len' blocks available.
// If no free spaces are availabe, then return -1
// If auto_set is set to true, will automatically mark the free blocks as taken
unsigned freemap_find_freespace(unsigned blk_len, bool auto_set){
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

				//unsigned result = (free_start_idx*8) + j - 1;
				unsigned result = (free_start_idx*8) + free_start_off;

				// Auto-set freemap
				if(auto_set == true){
					freemap_set(1, blk_len, result);
				}

				return result;
			}
		}

		free(bits);
	}

	// Couldn't find enough contiguous free spaces
	return -1;
}


