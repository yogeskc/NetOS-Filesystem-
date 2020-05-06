#pragma once

typedef struct{
    
    char *cmd_id;      //name of func
    int min_args;      // no. of minimum arguments
    char *description; //describe what it does
	char *arg_description;	// array of descriptions for each arg
    
} Finder;

void lsh_loop();
