#include <stdlib.h>
#include "commands.h" 
#include "low.h"
#include "utils.h"
#include <sys/wait.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 


//File Updated
 #define LSH_TOK_BUFSIZE 64
 #define LSH_TOKEN " \t\r\n\a"
    
uint64_t blockSize;
uint64_t volumeSize;

   int lsh_exit(char **args); 
    
 
    //listing the commands
    
    char *extra_str[]={
        "exit"
        "help"
    }; 
 
    int (*extra_func[])(char **) ={
        &lsh_exit
    };
 
    int lsh_nums(){
        return sizeof (extra_str)/sizeof(char *); 
 
    }
 
    //xtra function implementations
    int lsh_exit(char **args){
        return 0; 
    }
 
  
    
 
    int lsh_execute(char **args)
    {
      int i;
    
      if (args[0] == NULL) {
        // An empty command was entered.
        return 1;
      }
        for (i=0; i< lsh_nums();i++){
           if(strcmp(args[0],extra_str[i])== 0){
               return (*extra_func[i])(args);
           }
       }
      
    }
    
    #define LSH_RL_BUFSIZE 1024
    /**
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
    char **lsh_split_line(char *line)
    {
      int bufsize = LSH_TOK_BUFSIZE, position = 0;
      char **tokens = malloc(bufsize * sizeof(char*));
      char *token;
    
      if (!tokens) {
        fprintf(stderr, "ls: allocation error\n");
        exit(EXIT_FAILURE);
      }
    
      token = strtok(line, LSH_TOKEN);
      while (token != NULL) {
        tokens[position] = token;
        position++;
    
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
      char **args;
      int status;
    
      do {
        printf("> ");
        line = lsh_read_line();
        args = lsh_split_line(line);
        status = lsh_execute(args);
    
        free(line);
        free(args);
      } while (status);
    }
    

int main(int argc, char *argv[]){
	uint64_t blockSize = 512;
	uint64_t volumeSize = blockSize * 100;
         
  	lsh_loop();

	// create filesystem file
	startPartitionSystem ("test", &volumeSize, &blockSize);
     
	fs_add_file("test.txt",10,blockSize); 


	return 0;
}
