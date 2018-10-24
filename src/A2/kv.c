#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int kv_store_create(char *name);
int kv_store_write(char *key, char *value);
char *kv_store_read(char *key);
char **kv_store_read_all(char *key);

unsigned long hash(unsigned char *str){
    unsigned long hash = 0;
    int c;

    while(c = *str++)
        hash = c + (hash << 6) + (hash << 16) - hash;
    return (hash > 0) ? hash : -(hash);
}

int kv_store_create(char *name){

    char *str = name;
    int fd = shm_open("hqmyshared", O_CREAT | O_RDWR, S_IRWXU);

    char *addr = mmap(NULL, strlen(str), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, strlen(str));
    close(fd);

    memcpy(addr, str, strlen(str));
    munmap(NULL, fd);
}

int kv_store_write(char *key, char *value){

}

int main (int argc, char **argv){

}