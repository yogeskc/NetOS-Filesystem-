#include <stddef.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "shell.h"
#include "commands.h"
#include "utils.h"

// command data
Finder def_funcs[] = {
    {"add_file", 2},
    {"rm_file", 1},
    {"cp_file", 2},
    {"exit", 0}
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

    if(argc < f->min_args+1){
        printf("%s requires %d arguments\n", f->cmd_id, f->min_args);
        return 0;
    }

    // running associated function w/command
    if(strcmp(argv[0], "add_file") == 0){
        //return fs_add_file(argv[1], argv[2]);
        fs_add_file(argv[1], 10);
        return 0;
    }

    if(strcmp(argv[0], "exit") == 0){
        return -1;
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
