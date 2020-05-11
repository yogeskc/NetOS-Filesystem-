#include <stddef.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "commands.h"
#include "utils.h"

typedef struct{
    char *cmd_id;      //name of func
    int min_args;      // no. of minimum arguments
    char *description; //describe what it does
    char *arg_description;    // array of descriptions for each arg
} Finder;
 
//giving def of the struct Finder
Finder def_funcs[] = {
    {
		"exfile_add", 1, 
		"Add external file into netFS filesystem. File is placed in current directory",
		"\targ1 - path to external file to be read"
	},
    {
		"exfile_write", 2, "Write out internal file into external filesystem",
		"\targ1 - path to internal file to be read\n\targ2 - path to external file to be written out to"
	},
    {
		"rm", 1, 
		"Delete a target file from the filesystem",
		"\targ1 - path to target file (to be deleted)"
	},
	{
		"cp", 2, 
		"copy a target file into another directory",
		"\targ1 - path to target file (to be copied)\n\targ2 - path to destination directory"
	},
    {
		"mv", 2, 
		"move the files from one directory to another",
		"\targ1 - path to target directory (to be moved)\n\targ2 - path to destination directory"
	},
    {
		"ccd", 1, 
		"Creates a new directory within your current folder",
		"\targ1 - directory name"
	},
    {
		"cd", 1, 
		"Change the directory to a given location",
		"\targ1 - path to target directory"
	},
	{
		"rename", 2,
		"modify the name of a file or directory",
		"\targ1 - path to file to be renamed\n\targ2 - new name of file"
	},
    {
		"ls", 0, 
		"Lists content of current folder"
	},
    {
		"tree", 0, 
		"print out directories in tree format",
	},
    {
		"exit", 0, 
		"Quit netFS application"
	},
    {
		"help", 0, "Rescued Done. Hope it helps :)"
	}
};

#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOKEN " \t\r\n\a"

// find command based on name
Finder *lsh_find_func(char *name){
    for(int i = 0; i < sizeof(def_funcs) / sizeof(def_funcs[0]); i++){
        if(strcmp(name, def_funcs[i].cmd_id) == 0){
            return &def_funcs[i];
        }
    }

    printf("Command %s not found.\n", name);
    return NULL;
}

// command - help. Iterate all def_funcs and print info about them
void lsh_print_help(){
	printf("Rescue COMING..\n\n");

	for(int i = 0; i < sizeof(def_funcs) / sizeof(def_funcs[0]); i++){
		// Print command name and desc
        printf("[%s]: ", def_funcs[i].cmd_id);
        printf("%s\n", def_funcs[i].description);

		// Print command arg description
		if(def_funcs[i].min_args <= 0){
			printf("\targs: none\n");
		}else{
			printf("%s\n", def_funcs[i].arg_description);
		}

		printf("\n");
	}    
}

// run an FS command based on user input
int lsh_parse_input(int argc, char **argv){
    if(argc <= 0){
        return 0;
    }

    // Get info about the command
    Finder *f = lsh_find_func(argv[0]);
    if(f == NULL){
        return 0;
    }

	// Check if command meets minimum args
    if(argc < f->min_args+1){
        printf("%s requires %d arguments\n", f->cmd_id, f->min_args);
        return 0;
    }

	// Run associated function!
	if(strcmp(argv[0], "exfile_add") == 0){
		exfile_add(argv[1], fs_get_cur_dir());
	}

	if(strcmp(argv[0], "exfile_write") == 0){
		exfile_write(argv[1], argv[2], fs_get_cur_dir());
	}

    if(strcmp(argv[0], "rm") == 0){
		file_remove(argv[1], fs_get_cur_dir());
	}

    if(strcmp(argv[0], "cp") == 0){
		file_copy(argv[1], argv[2], fs_get_cur_dir());
	}

    if(strcmp(argv[0], "mv") == 0){
		file_move(argv[1], argv[2], fs_get_cur_dir());
	}

	if(strcmp(argv[0], "ccd") ==0){ 
		dir_create( argv[1] ,fs_get_cur_dir());	
	}

	if(strcmp(argv[0], "cd") ==0){ 
		fs_change_dir (argv[1]);	
	}

	if(strcmp(argv[0], "rename") ==0){
		entry_rename(argv[1], argv[2], fs_get_cur_dir());
	}

    if(strcmp(argv[0], "ls") ==0){ 
		dir_list(fs_get_cur_dir());  
	}

    if(strcmp(argv[0], "tree") ==0){
		dir_tree(fs_get_cur_dir(), 0);
	}

    if(strcmp(argv[0], "exit") == 0){
        return -1;
    }

    if(strcmp(argv[0], "help") ==0){
		lsh_print_help();
    }
    
    return 0;
}


/*
brief Read a line of input from stdin.
return The line from stdin.
*/
char *lsh_read_line(void)
{
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Read a character
        c = getchar();

        // If we hit EOF, replace it with a null character and return.
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        // If we have exceeded the buffer, reallocate.
        if (position >= bufsize) {
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

//splits line into tokens
char **lsh_split_line(char *line, int *count)
{
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "ls: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOKEN);
    *count = 0;
    while (token != NULL) {
        tokens[position] = token;
        position++;
        *count += 1;

        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "ls: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOKEN);
    }
    tokens[position] = NULL;
    return tokens;
}

//MAIN FUNCTION FOR THE PROGRAM
void lsh_loop(void)
{
    char *line;
    int status;

	printf("Type 'help' to print a list of commands\n");

    do {
        printf("> ");
        line = lsh_read_line();

        if(strlen(line) == 0) continue;

        // get argc, argv from user input
        int argc;
        char **argv = get_args(line, &argc);

        // run user command
        status = lsh_parse_input(argc, argv);

        free(line);
        free(argv);
    } while (status >= 0);
}

//Driver
int main(int argc, char *argv[]){
    //taking a path to a file doesn't exist. ./netfs will automatically create a file call 'test'. A blank file that is, whenever it's created or listed, it's happening within that file.
	
	// Choose filesystem file name. Default: data_fs 
	char *filesystem_name = "data_fs";
	if(argc > 1)
		filesystem_name = argv[1];

	// Create/load the filesystem file.
    if(fs_start(filesystem_name) == -1)
		return -1;
    
    lsh_loop();

    fs_close();

    return 0;
}
