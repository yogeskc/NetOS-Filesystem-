#include "fsLow.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
	uint64_t blockSize = 512;
	uint64_t volumeSize = blockSize * 100;

	// create filesystem file
	startPartitionSystem ("test", &volumeSize, &blockSize);

	// 01101000 01100101 01101100 01101100 01101111
	char* test_write = "hello";
	LBAwrite( (void *) test_write, 1, 10);

	void *test_read = malloc( blockSize );
	LBAread(test_read, 1, 10);

	printf("%s\n", (char *)test_read);

	return 0;
}
