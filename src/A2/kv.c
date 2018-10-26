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

//array of pods
//curr pair key/values
typedef struct kv_pair{
    char *key;
    char *value;
} kv_pair;

typedef struct pod{
    int index;
    kv_pair *kv_pair;
} pod;

typedef struct kv_info{
    char *kv_name;
    pod pods[128];
} kv_info;

kv_info *this_kv_info;

unsigned long hash(unsigned char *str){
    unsigned long hash = 0;
    int c;

    while(c = *str++)
        hash = c + (hash << 6) + (hash << 16) - hash;
    return (hash > 0) ? hash : -(hash);
}

int kv_store_create(char *name){
    // create the kv at the designated name
    char *str = name;
    int fd = shm_open(name, O_CREAT | O_RDWR, S_IRWXU);
    
    if (fd < 0){
       printf("Failed to create a KV");
       return -1;
    }

    this_kv_info = mmap(NULL, strlen(str), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, strlen(str));
    close(fd);
    // memcpy(addr, str, strlen(str));
    munmap(NULL, fd);
    return 0;
}

int kv_store_write(char *key, char *value){

    struct stat s;

    char *str = key;
    int fd = shm_open(kv_name, O_RDWR, S_IRWXU);
    if (fd < 0){
       printf("Failed to write to KV \n");
       return -1;
    }

    if(fstat(fd, &s) == -1){
        printf("Failed to fstat \n");
        return -1;
    }
    
    unsigned long hashed = hash(key);
    char *addr = mmap(hashed, s.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, strlen(str));
    close(fd);
    memcpy(addr, str, strlen(str));
    munmap(hashed, fd);
    return 0;
}

char *kv_store_read(char *key){
    struct stat s;

    char *str = key;
    int fd = shm_open("hqmyshared",O_RDWR, S_IRWXU);
    char *addr = mmap(NULL, strlen(str), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    unsigned long hashed = hash(key);
}

int main (int argc, char **argv){

}