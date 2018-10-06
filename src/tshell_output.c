#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define _GNU_SOURCE
#include <linux/sched.h>
#include <signal.h>
#include <fcntl.h>


void my_systemf(char *command){
    int pid = fork();
    if(pid == 0){
        char* myfifo = "/tmp/myfifo"; 
        mkfifo(myfifo, 0666); 
        close(0); 
        open(myfifo, O_RDONLY);
        execl("/bin/sh", "sh", "-c", command, NULL);
        close(0);
    }
    else{
        wait(NULL);
    }
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
            my_systemf(line);
        else    
            return -1;
    }
}