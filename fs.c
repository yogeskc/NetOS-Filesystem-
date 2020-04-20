#include "commands.h"
#include "low.h"
#include "shell.h"
#include "utils.h"

int main(int argc, char *argv[]){
    uint64_t blockSize = 512;
    uint64_t volumeSize = blockSize * 100;

    // Init filesystem
    startPartitionSystem ("test", &volumeSize, &blockSize);

    // CLI loop
    lsh_loop();

    return 0;
}
