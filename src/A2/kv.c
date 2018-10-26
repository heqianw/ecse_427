#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

int kv_store_create(char *name);
int kv_store_write(char *key, char *value);
char *kv_store_read(char *key);
char **kv_store_read_all(char *key);
sem_t *my_sem;

//array of pods
//curr pair key/values
typedef struct kv_pair{
    char *key;
    char *value;
} kv_pair;

typedef struct pod{
    int index;
    kv_pair kv_pairs[128];
} pod;

typedef struct kv_info{
    char *kv_name;
    pod pods[128];
} kv_info;

kv_info *this_kv_info;
char *shmname;

unsigned long hash(unsigned char *str){
    unsigned long hash = 0;
    int c;

    while(c = *str++)
        hash = c + (hash << 6) + (hash << 16) - hash;
    return (hash > 0) ? hash : -(hash);
}

// unsigned long hash(unsigned char *str){
//     unsigned long hash = 5381;
//     int c;

//     while(c = *str++)
//         hash = c + ((hash << 5) + hash);
//     return (hash > 0) ? hash : -(hash);
// }

int kv_store_create(char *name){
    // create the kv at the designated name
    int fd = shm_open(name, O_CREAT | O_RDWR, S_IRWXU);
    
    if (fd < 0){
       printf("Failed to create a KV");
       return -1;
    }
    shmname = name;
    this_kv_info = mmap(NULL, sizeof(kv_info), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, sizeof(kv_info));
    // memcpy(addr, str, strlen(str));
    this_kv_info -> kv_name;
    munmap(this_kv_info, sizeof(kv_info));
    close(fd);
    return 0;
}

int kv_store_write(char *key, char *value){

    int fd = shm_open(shmname, O_RDWR, S_IRWXU);
    if (fd < 0){
       printf("Failed to write to KV \n");
       return -1;
    }

    // if(fstat(fd, &s) == -1){
    //     printf("Failed to fstat \n");
    //     return -1;
    // }

    this_kv_info = mmap(NULL, sizeof(kv_info), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, sizeof(kv_info));
    printf("does this reach");
    my_sem = sem_open("/semHQ",O_CREAT | O_RDWR, S_IRWXU, 1);
    sem_wait(my_sem);
    
    int podIndex = hash(key) % 128;
    pod *actualPod = & (this_kv_info ->pods[podIndex]);
    int actualIndex = (actualPod -> index);
    kv_pair *actualPair = & (actualPod -> kv_pairs[actualIndex]);
    
    memcpy(&actualPair->key, key, strlen(key));
    memcpy(&actualPair->value, value , strlen(value));

    // actualPod -> index = (int)

    sem_post(my_sem);

    
    munmap(this_kv_info, sizeof(kv_info));
    close(fd);
    return 0;
}

// char *kv_store_read(char *key){
//     struct stat s;

//     char *str = key;
//     int fd = shm_open("hqmyshared",O_RDWR, S_IRWXU);
//     char *addr = mmap(NULL, strlen(str), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//     unsigned long hashed = hash(key);
// }

int main (int argc, char **argv){
    printf("create");
    kv_store_create(argv[1]);
    printf("Create is done");
    // kv_store_write(argv[2], argv[3]);
}