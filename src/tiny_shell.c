#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/wait.h>
#define _GNU_SOURCE
#include <linux/sched.h>
#include <signal.h>

void my_systemf(char *command){
    int pid = fork();
    if(pid == 0){
        execl("/bin/sh", "/bin/sh", "-c", command, NULL);
    }
    else{
        wait(NULL);
    }
}

void my_systemv(char *command){
    int pid = vfork();
    if(pid == 0){
        execl("/bin/sh", "/bin/sh", "-c", command, NULL);
    }
    else if(pid < 0){
        return;
    }
    else{
        wait(NULL);
    }
}
int fn (char *command){
    execl("/bin/sh", "/bin/sh", "-c", command, NULL);
    return 0;
}

void my_systemc(char *command){
    void *pchild_stack = malloc(1024 * 1024);
    int pid = clone(fn, pchild_stack + (1024 * 1024), CLONE_FS | SIGCHLD, command);
    if ( pid < 0 ) {
        printf("ERROR: Unable to create the child process.\n");
        exit(EXIT_FAILURE);
    }
    wait(NULL);
    free(pchild_stack);
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
            my_systemc(line);
        else    
            return -1;
    }
}