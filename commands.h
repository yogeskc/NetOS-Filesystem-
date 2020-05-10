#pragma once

#include <stdlib.h>
#include "utils.h"

/* 
 * Entry - Storage object which points to either a Directory or file
 *
 * if Entry is a file, blk_data points to the start block of the file's raw data
 * if Entry is a directory, blk_data points to the associated Directory block
 * if Entry is a directory, it's size is 0
 *
 * size is counted in bytes
 *
 * blk_next points to the next Entry within the Directory's entry chain
 * blk_next equals -1 at the end of the chain
 *
 * metadata is contained directly in the Entry struct
 */
typedef struct {
	unsigned blk_data; 	
	unsigned blk_next;	
	unsigned size; 
	char name[256];
	int is_dir;
	unsigned time_created;
} Entry;

/* 
 * Directory - Storage object which points to the beginning of an entry chain
 *
 * Directories are sandwiched between two linking entries:
 * - the (..) entry, which points back to the container dir
 * - the link entry, which resides within the container dir's entry chain
 *
 * Many of the FS functions are designed around taking a directory block
 * (usually called blk_container), and then iterating over it's directory chain
 *
 * The global variable 'g_cur_dir' stores the pointer to the current Directory block.
 * This can intuitively be used for functions within commands.h. For example,
 *
 * dir_list(g_cur_dir) will list your current directory, and 
 * dir_find_entry("name", g_cur_dir) searches your current directory for an entry.
 */
typedef struct{
	unsigned blk_start; 
} Directory;

/* 
 * Superblock - Holds global filesystem info
 *
 * The superblock is ALWAYS written to block 0
 * when the filesystem is started, it's loaded as a global variable 'g_super'
 * len_freemap stores the count of freemap blocks (stored after the superblock).
 *
 */
typedef struct{
	unsigned blk_root; 		
	unsigned blk_freemap;	
	unsigned len_freemap;
} Superblock;

/* 
 * Create a new directory within blk_container
 * params - new dir name, block of container dir
 * returns - block location of new directory
*/
unsigned dir_create (char *name, unsigned blk_container);
unsigned dir_create_root ();	

unsigned dir_move(char *path_src, char *path_dest, unsigned blk_container);
int dir_rm(char *path_src, unsigned blk_container);

/*
 * Iterate across an entry chain, print all entry names
 * params - block of directory to iterate
 */
int dir_list (unsigned blk_container);

/* 
 * Recursive function which prints the directories in a 
 * tree format. Deeper levels are tabbed inwards using
 * |_ symbols, to help visualize how the tree looks.
 *
 * dir_tree should be called with level=0 (this param is used in the recursion)
 */
int dir_tree (unsigned blk_container, int level);

/*
 * Search a directory for a given entry matching "name"
 * params - the search name, and container dir
 *
 * the 'before' parameter defines whether or not to 
 * return the entry preceeding the search result.
 *
 * returns - the block location of the search result entry. -1 if it doesnt exist.
 */
unsigned dir_find_entry (char *name, unsigned blk_container);

/* 
 * Follow a entry chain until the end is reached,
 * return block location of final entry. -1 on error.
 */
unsigned dir_find_end (unsigned blk_container);	

/* 
 * Load a target block into a Directory struct,
 * return a malloc'd struct filled with the target directory's data
 */
Directory *dir_load (unsigned block);			

/* 
 * Load a target block into an Entry struct,
 * return a malloc'd struct filled with the target entry's data
 */
Entry *entry_load (unsigned block);			

/*
 * Seek to the end of an entry chain, and 
 * point the final Entry to a new block location
 * params - block of container directory, and new block location to point to
 */
int entry_chain_append(unsigned blk_container, unsigned entry_ptr);

/*
 * Analyze a given filepath and search for an associated Entry.
 * 
 * The function splits apart the path parameter by slashes, 
 * iterating through each directory and checking if the next
 * part exists. This repeats until the end is reached.
 *
 * the 'dir' variable is the directory where the search begins.
 * if the path begins with a slash, 'dir' is immediately set to root
 *
 * examples of valid path arguments:
 *
 * /images/family/2020.png
 * music/albums/pink_floyd/
 * ../../folder/hello.txt
 * 
 * returns - block location of search entry result, -1 if it doesnt exist.
 * for directories, the link Entry is returned (the entry within the container)
 * for files, the plain Entry is returned.
 */
unsigned resolve_path(char *path, unsigned dir);

//int file_rm (char *name, unsigned blk_container);
//int file_move (char *path_src, char *path_dest, unsigned blk_container);	

/*
 * Resolve path parameter into an entry, 
 * if it exists, replace name and save changes to disk.
 *
 * names with slash (/) characters are not accepted
 * you cannot rename (/) the root dir
 * you cannot rename (..) directories
 */ 
int file_rename (char *path, char *new_name, unsigned blk_container);	

/*
 * Search for an external file (outside of NetFS)
 * if it exists, split it into blocks and store it within the filesystem
 *
 * params - container block to add the new file into 
 */ 
int exfile_add (char *path_ext, unsigned blk_container);

/* 
 * Resolve the path_int param into an entry,
 * if it exists, write the file's data to the external filesystem (outside of NetFS)
 * params - path for internal and external (Dest) file, and the container
 */
int exfile_write (char *path_int, char *path_ext, unsigned blk_container);

/*
 * Resolve the path into an entry
 * if it exists, modify the filesystem's current directory
 *
 * this func sets the global g_cur_dir variable
 */
int fs_change_dir (char *path);		

/*
 * Returns block location of filesystem current directory (default: root)
 */
unsigned fs_get_cur_dir ();

/*
 * Start the filesystem. 
 *
 * 'filename' is the path to the external netFS filesystem file.
 * if the filename doesnt exist, a new, blank filesystem will be created.
 * if the filename does exist, the program will attempt to load the netFS filesystem from it.
 *
 */
int fs_start (char *filename);

/*
 * Cleanup the filesystem
 *
 * Free all globals allocated in fs_start() 
 * aka: freemap, superblock, etc
 */
void fs_close ();
