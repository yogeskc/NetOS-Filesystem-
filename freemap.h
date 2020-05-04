#pragma once
#include "utils.h"

// Create a blank freemap
void freemap_create();

// Load freemap from disk
void freemap_load(long ptr_freemap, long len_freemap);

// Save freemap to disk
void freemap_save();

// Free current freemap memory
void freemap_cleanup();

// return dynamic freemap length (Full size)
int freemap_get_start();
int freemap_get_len();

// modify the freemap, set multiple blocks to true or false
// params
// taken - set the target blocks to either 1 or 0
// blk_len - how many blocks from the offset to modify at
// blk_start - offset within the freemap to start modifying at
void freemap_set(bool taken, long blk_len, long blk_start);

// search the freemap for the first occurance of contiguous freespace, with length 'blk_len'
// blk_len - minimum length of free blocks in a row
// auto_set - auto-run freemap_set on the newly found blocks
long freemap_find_freespace(long blk_len, bool auto_set);
