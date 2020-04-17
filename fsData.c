#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fsData.h"
#include "fsLow.h"
#include "fsUtils.h"

// take a file from outside the NetOS filesystem and copy it in
void copy_file(char *filepath, unsigned data_start, uint64_t blockSize){
	void *buffer = read_file(filepath);

	if(buffer == NULL){
		printf("Error: %s file doesn’t exist!\n", filepath);
		return;
	}

	Entry *new_entry = malloc(sizeof(Entry*));
	strcpy(new_entry->d_name, filepath);
	new_entry->d_reclen = round(sizeof(buffer) / blockSize); // always round up!!!

	Nugget new_nug;
	new_nug.ptr_data_start = data_start+1;

	new_entry->nug_cur = new_nug;
	//new_entry.nug_next = NULL;

	LBAwrite( buffer, new_entry->d_reclen, new_nug.ptr_data_start);  //stores raw data in test.txt
	LBAwrite( (void *)new_entry, 1, data_start); // putting an entry
}

void *readfile(unsigned data_start, uint64_t blockSize){

        void *buffer = malloc(blockSize); 
        
	LBAread(buffer,1,data_start); 
       
  	//Convert raw data into Entry
	Entry* entry = (Entry*) buffer; 
        
	//Allocate space for the data which the Entry points to 
   	void* entry_data = malloc(entry-> d_reclen* blockSize); 

	//load the data associated with the Entry 
	LBAread(entry_data, entry->d_reclen,entry->nug_cur.ptr_data_start); 

	free(buffer); 
	return entry_data; 
}

