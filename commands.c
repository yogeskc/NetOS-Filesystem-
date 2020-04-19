#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "low.h"
#include "utils.h"
//updated

// take a file from outside the NetOS filesystem and copy it in
void fs_add_file(char *filepath, unsigned data_start, unsigned blockSize){
	void *buffer = get_file_data(filepath);
	int buf_size = get_file_size(filepath);

	if(buffer == NULL){
		printf("Error: %s file doesn’t exist!\n", filepath);
		return;
	}

	// Create new entry
	Entry *new_entry = (Entry *) malloc(sizeof(Entry));
	strcpy(new_entry->d_name, filepath);
	new_entry->d_name[sizeof(new_entry->d_name)-1] = '\0';

	// Detect record length and allocate blocks
	new_entry->d_reclen = round(buf_size / blockSize); // always round up!!!

	if(new_entry->d_reclen == 0){
		new_entry->d_reclen = 1;
	}

	printf("Allocating %d blocks in filesystem\n", new_entry->d_reclen);

	// create nuggets
	Nugget nug_cur;
	Nugget nug_next;
	nug_cur.ptr_data_start = data_start+1;
	nug_next.ptr_data_start = -1;

	// set nuggets
	new_entry->nug_cur = nug_cur;
	new_entry->nug_next = nug_next;

	LBAwrite( buffer, new_entry->d_reclen, nug_cur.ptr_data_start);  //stores raw data from file
	LBAwrite( (void *)new_entry, 1, data_start); // putting an entry

	free(buffer);
	free(new_entry);
}

// Load the directory entry at data_start
void *fs_read_file(unsigned data_start, unsigned blockSize){
		void *buffer = malloc(blockSize);

		// Load the directory entry @ data_start
		LBAread(buffer, 1, data_start);

		// Convert raw data into Entry
		Entry *entry = (Entry *)buffer;

		// Allocate space for the data which the Entry points to
		void *entry_data = malloc(entry->d_reclen * blockSize);

		// Load the data associated with the Entry
		LBAread( entry_data, entry->d_reclen, entry->nug_cur.ptr_data_start);

		free(buffer);
		return entry_data;
}
