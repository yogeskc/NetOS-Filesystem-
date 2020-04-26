#include <stdio.h>

#include "commands.h"
#include "low.h"
#include "shell.h"
#include "utils.h"

extern Superblock *g_super;

int main(int argc, char *argv[]){
	fs_start("test");
	
	// print root dir
	Directory *root = dir_load(g_super->ptr_root);
	dir_list(root);
	free(root);

	fs_close();
	return 0;
}
