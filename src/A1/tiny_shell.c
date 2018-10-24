#define _GNU_SOURCE
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/sched.h>
#include <signal.h>

double gettime(){
    struct timespec ts;
    if(clock_gettime(CLOCK_REALTIME, &ts) < 0)
        perror("Clock-gettime");
    return ts.tv_sec * 10000000 + ts.tv_nsec/1000;
}


int fn (char *command){
    execl("/bin/sh", "sh", "-c", command, NULL);
    printf("Clone: child");
    return 0;
}


void my_system(char *command){
    #ifdef FORK
        int status;
        int pid = fork();
        if(pid == 0){
            execl("/bin/sh", "sh", "-c", command, NULL);
        }
        else {
            waitpid(pid, &status, 0);
        }

    #elif VFORK
        int status;
        int pid = vfork();
        if(pid == 0){
            execl("/bin/sh", "sh", "-c", command, NULL);
        }
        else {
            waitpid(pid, &status, 0);
        }
    #elif CLONE
        void *pchild_stack = malloc(1024 * 1024);
        int pid = clone(fn, pchild_stack + (1024 * 1024), CLONE_FS | SIGCHLD, command);
        if ( pid < 0 ) {
            printf("ERROR: Unable to create the child process.\n");
            exit(EXIT_FAILURE);
        }
        wait(NULL);
        free(pchild_stack);
    #else
        printf("Wrong flags for the compilation");
    #endif

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
        if (length(line) > 1){
            double start = gettime();
            my_system(line);
            double end = gettime();
            printf("The time taken is %f us. \n", end - start);
        }
        else    
            return -1;
    }
}