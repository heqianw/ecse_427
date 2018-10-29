#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
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
    char key[32];
    char value[256];
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

    this_kv_info = mmap(NULL, sizeof(kv_info), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, sizeof(kv_info)+sizeof(char));
    // printf("does this reach");
    my_sem = sem_open("/semHQ",O_CREAT | O_RDWR, S_IRWXU, 1);
    sem_wait(my_sem);
    
    //this is the critical section
    int podIndex = hash(key) % 128;
    pod *actualPod = & (this_kv_info -> pods[podIndex]);
    int actualIndex = (actualPod -> index);
    kv_pair *actualPair = & (actualPod -> kv_pairs[actualIndex]);
    
    memcpy(&actualPair -> key, key, strlen(key));
    memcpy(&actualPair -> value, value , strlen(value) + 1);

    actualIndex++;
    if(actualIndex > 127){
        actualPod -> index = actualIndex % 128;    
    }
    else{
        actualPod -> index = actualIndex;
    }
    // end of critical section
    // actualPod -> index = (int)

    sem_post(my_sem);

    printf("Pod number %d \n", podIndex);
    printf("Index of pod is %d\n", actualIndex);
    // find a way to update the index
    
    munmap(this_kv_info, sizeof(kv_info));
    close(fd);
    return 0;
}

//once we compute hash and we get to the hash, parse the list of values for the key that we want, by doing key equality
char *kv_store_read(char *key){
    int fd = shm_open(shmname, O_RDWR, S_IRWXU);
    if (fd < 0){
       printf("Failed to write to KV \n");
       return NULL;
    }

    this_kv_info = mmap(NULL, sizeof(kv_info), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, sizeof(kv_info)+sizeof(char));

    // printf("does this reach");
    int i;
    char *valueToReturn;
    int found = 0;

    my_sem = sem_open("/semHQ",O_CREAT | O_RDWR, S_IRWXU, 1);
    sem_wait(my_sem);

    int podIndex = hash(key) % 128;
    pod *actualPod = & (this_kv_info -> pods[podIndex]);
    int numberEntries = (actualPod -> index);
    

    printf("The pod number is %d\n", podIndex);
    printf("Found is %d\n", found);
    for(i = 0; i < numberEntries; i++){
        kv_pair *currentPair = &(actualPod -> kv_pairs[i]);
        char *storedKey = (currentPair -> key);
        printf("Key is %s\n", key);
        printf("StoredKey is %s\n", storedKey);
        if(strcmp(key, storedKey) == 0){
            printf("This is true\n");
            valueToReturn = strdup((currentPair -> value));
            found = 1;
        }
    }
    printf("Value Return is %s\n", valueToReturn);
    sem_post(my_sem);
    munmap(this_kv_info, sizeof(kv_info));
    close(fd);
    printf("Found is %d\n", found);
    
    if(found == 1){
        return valueToReturn;
    }
    else{
        return NULL;
    }
}

// instead of doing what we did earlier and compare the key, if hash function has same thing, just return all values in a char array
char **kv_store_read_all(char *key){

    char **allValues = malloc(sizeof(char*));
    
    int fd = shm_open(shmname, O_RDWR, S_IRWXU);
    if (fd < 0){
       printf("Failed to write to KV \n");
       return NULL;
    }

    this_kv_info = mmap(NULL, sizeof(kv_info), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, sizeof(kv_info) + sizeof(char));

    // printf("does this reach");
    int i;
    char *valueToReturn;
    int found = 0;
    int count = 0;
    my_sem = sem_open("/semHQ",O_CREAT | O_RDWR, S_IRWXU, 1);
    sem_wait(my_sem);

    int podIndex = hash(key) % 128;
    pod *actualPod = & (this_kv_info -> pods[podIndex]);
    int numberEntries = (actualPod -> index);
    
    //loop to tell us how many values for key
    for(i = 0; i < numberEntries; i++){
        kv_pair *currentPair = &(actualPod -> kv_pairs[i]);
        char *storedKey = (currentPair -> key);
        if(strcmp(key, storedKey) == 0){
            printf("This is true\n");
            count++;
            found = 1;
        }
    }
    //create 2D char array for the number of values, then loop to add to this 2D char array

    sem_post(my_sem);
    munmap(this_kv_info, sizeof(kv_info));
    close(fd);
    printf("Found is %d\n", found);
    
    if(found == 1){
        return valueToReturn;
    }
    else{
        return NULL;
    }
}

int main (int argc, char **argv){
    // printf("create");
    kv_store_create(argv[1]);
    // kv_store_write(argv[2], argv[3]);
    printf("%s", kv_store_read("testKey"));
}