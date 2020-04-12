#include "fsLow.h"
#include <stdio.h>
#include <stdlib.h>

#define BLOCKSIZE 512

int main(){
	// create filesystem file
	startPartitionSystem ("test", (1024 * 100), BLOCKSIZE);

	// 01101000 01100101 01101100 01101100 01101111
	char* test_write = "hello";
	LBAwrite( (void *) test_write, 1, 10);

	void *test_read = malloc( BLOCKSIZE );
	LBAread(test_read, 1, 10);

	printf("%s\n", (char *)test_read);

	return 0;
}
