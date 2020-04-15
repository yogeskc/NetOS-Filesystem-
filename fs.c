#include <stdio.h>
#include <stdlib.h>

#include "fsLow.h"
#include "fsUtils.h"

uint64_t blockSize;
uint64_t volumeSize;


int main(int argc, char *argv[]){
	uint64_t blockSize = 512;
	uint64_t volumeSize = blockSize * 100;

	// create filesystem file
	startPartitionSystem ("test", &volumeSize, &blockSize);

	return 0;
}
