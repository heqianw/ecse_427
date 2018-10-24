#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#define _GNU_SOURCE
#include <sched.h>


int fn(void *arg)
{
   printf("\nINFO: This code is running under child process.\n");

   int i = 0;
   
   int n = atoi(arg);

   for ( i = 1 ; i <= 10 ; i++ )
      printf("%d * %d = %d\n", n, i, (n*i));

   printf("\n");

   return 0;
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


void main(int argc, char *argv[])
{
    printf("Hello, World!\n");

    void *pchild_stack = malloc(1024 * 1024);
    if ( pchild_stack == NULL ) {
        printf("ERROR: Unable to allocate memory.\n");
        exit(EXIT_FAILURE);
    }
    while(1){
        char *line = get_a_line();
        if (length(line) > 1){
            int pid = clone(fn, pchild_stack + (1024 * 1024), SIGCHLD, argv[1]);

            if ( pid < 0 ) {
                printf("ERROR: Unable to create the child process.\n");
                exit(EXIT_FAILURE);
            }

        wait(NULL);
        free(pchild_stack);
        }
    }
    printf("INFO: Child process terminated.\n");
}

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


int fn(void *command)
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

void main(int argc, char *argv[])
{
   printf("Hello, World!\n");

   void *pchild_stack = malloc(1024 * 1024);
   if ( pchild_stack == NULL ) {
      printf("ERROR: Unable to allocate memory.\n");
      exit(EXIT_FAILURE);
   }

   int pid = clone(fn, pchild_stack + (1024 * 1024), SIGCHLD, argv[1]);
   if ( pid < 0 ) {
        printf("ERROR: Unable to create the child process.\n");
        exit(EXIT_FAILURE);
   }

   wait(NULL);

   free(pchild_stack);

   printf("INFO: Child process terminated.\n");
}