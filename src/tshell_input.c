#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <linux/sched.h>
#include <signal.h>

#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 

void my_systemf(char *command){
    int pid = fork();
    if(pid == 0){
        execl("/bin/sh", "sh", "-c", command, NULL);
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
    int fd; 
    char * myfifo = "/tmp/myfifo";  
    mkfifo(myfifo, 0666); 
  
    char arr1[80], arr2[80]; 

    while(1){
        fd = open(myfifo, O_WRONLY); 
        char *line = get_a_line();
        if (length(line) > 1){
            close(1);
            fd = open(myfifo, O_WRONLY);
            my_systemf(line);
        }
        else    
            return -1;
    }
}