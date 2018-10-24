#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


void main(int argc, char *argv[]){
    for(int i = 0; i < 4; i++){
        int pid = fork();
        if(pid == 0){
            printf("%d \n", i);
        }
        else{
            wait(NULL);
        }
    }
}