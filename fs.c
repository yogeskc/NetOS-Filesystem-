#include <stdio.h>

#include "commands.h"
#include "low.h"
#include "shell.h"
#include "utils.h"

extern Superblock *g_super;

int main(int argc, char *argv[]){
	fs_start("test");
	
	// print root dir
	//Directory *root = dir_load(g_super->ptr_root);
	//dir_list(g_super->ptr_root);
	//dir_create("testing", fs_get_cur_dir());//returns the block number 							of the current directory
	
	lsh_loop();
        //fs_change_dir ("testing");	
        //dir_list(fs_get_cur_dir());
	
        fs_close();
	
	
	return 0;
}
