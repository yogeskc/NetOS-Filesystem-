#include <stdio.h>

#include "commands.h"
#include "low.h"
#include "shell.h"
#include "utils.h"

int main(int argc, char *argv[]){
    uint64_t blockSize = BLOCKSIZE;
    uint64_t volumeSize = BLOCKSIZE * BLOCKCOUNT;

    // Init filesystem
    startPartitionSystem ("test", &volumeSize, &blockSize);
    freemap_init();
    freemap_cleanup();
    //printf("%d\n", sizeof(Entry));

    char test = (char)0x11111111;

    printf("%c\n", test);

    // CLI loop
    //lsh_loop();
    return 0;
}
