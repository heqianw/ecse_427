#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int my_system(char *command){
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
    for(i = 0; i < argCount; i++){
        printf("%s\n", argv[i]);
    }
    strcpy(progpath, path);
    strcat(progpath, argv[0]);

    for(i = 0; i < strlen(progpath); i++){
        if(progpath[i] == '\n'){
            progpath[i] = '\0';
        }
    }
    
    int pid = fork();
    if(pid == 0){
        execvp(progpath, argv);
    }
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

int main(int argc, char *argv[]){
    while(1){
        char *line = get_a_line();
        if (length(line) > 1)
            system(line);
        else    
            return -1;
    }
}