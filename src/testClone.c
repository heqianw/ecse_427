#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/wait.h>
#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>


int clone_function(void *command)
{
    printf("\nINFO: This code is running under child process.\n");
    char* argv[50];
    char *list = strtok(command, " ");
    char* path = "/bin/";
    char progpath[20];
    int i = 0;
    while(list != NULL && i < 50){
        argv[i] = list;
        list = strtok(NULL, " ");
        i++;
    }
    argv[i] = NULL;
    int argCount = i;
    strcpy(progpath, path);
    strcat(progpath, argv[0]);

    for(i = 0; i < strlen(progpath); i++){
        if(progpath[i] == '\n'){
            progpath[i] = '\0';
        }
    }
    execvp(progpath, argv);
    return 0;
}

void my_systemc(char *command)
{
   printf("Hello, World!\n");

   void *pchild_stack = malloc(1024 * 1024);
   if ( pchild_stack == NULL ) {
      printf("ERROR: Unable to allocate memory.\n");
      exit(EXIT_FAILURE);
   }

   int pid = clone(clone_function, pchild_stack + (1024 * 1024), SIGCHLD, command);
   if ( pid < 0 ) {
        printf("ERROR: Unable to create the child process.\n");
        exit(EXIT_FAILURE);
   }
   wait(NULL);
   free(pchild_stack);
   printf("INFO: Child process terminated.\n");
}

int length(char *s){
    int x = 0;
    while(s[x] != '\0')
        x++;
    return x;
} 

char *get_a_line(){
    char *line = NULL;
    size_t bufsize = 0;
    getline(&line, &bufsize, stdin);
    return line;
}

int main(int argc, char *argv[]){
    while(1){
        char *line = get_a_line();
        if (length(line) > 1)
            //my_systemf(line);
            my_systemc(line);
        else    
            return -1;
    }
}