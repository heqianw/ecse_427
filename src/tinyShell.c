#include <stdio.h>

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
            exit(0);
    }
}