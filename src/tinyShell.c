#include <stdio.h>

int length(char *string){
    int x = 0;
    return x;
} 

char *get_a_line(){
    
}

int main(int argc, char *argv[]){
    while(1){
        int line = get_a_line();
        if (length(line) > 1)
            system(line);
        else    
            exit(0);
    }
}