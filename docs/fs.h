//file name was a.h

//Chris Riddle
//Yangesh KC
//Shiuan Chen
//Group - NetOS

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>

struct DirEntry {
    id_t   ID; // ID of dirEntry
    time_t date_created;
    time_t date_modified;
    long parent_ptr;
    long startblock;
} ;

// This is our nugget struct
struct Nugget {
    long ptr_nuggets; //point to file metadata
    char name [128];
    long value;
};

//struct of file system info
struct FileSystemInfo {
    id_t VolumeID; // ID of Volume
    char *free; //location of the free space. We will use the pointer to point the location
    char *root; //location of the root directory
    char volname[20]; // name of volume
    char volID[20]; //volume ID
    int lbasize; //size of lba per block

};

#define FREE_SIG1 "NetOSFS!"
#define FREE_SIG2 "NetOSFS!"

// We will be using linked list to look for free space in the memory block

struct FreeSpace {
    char sig1[8];  // the magic number to start on the first block
    char sig2[8]; //the magic number at the bottom on the last block
    char buffer[512];
    long ptr_prev; // pointer to the previous linked list
    long ptr_next; // pointer to the next linked list
};
