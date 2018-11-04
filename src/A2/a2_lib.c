#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include "a2_lib.h"
int kv_store_create(char *name);
int kv_store_write(char *key, char *value);
char *kv_store_read(char *key);
char **kv_store_read_all(char *key);
sem_t *my_sem;

kv_info *this_kv_info;
char *shmname;

unsigned long hash(unsigned char *str){
    unsigned long hash = 0;
    int c;

    while(c = *str++)
        hash = c + (hash << 6) + (hash << 16) - hash;
    return (hash > 0) ? hash : -(hash);
}

int kv_store_create(char *name){
    // create the kv at the designated name
    int fd = shm_open(__KV_SHM_NAME__, O_CREAT | O_RDWR, S_IRWXU);
    
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
    int i;
    int keyExists = 0;
    int fd = shm_open(__KV_SHM_NAME__, O_RDWR, S_IRWXU);
    if (fd < 0){
       printf("Failed to write to KV \n");
       return -1;
    }

    this_kv_info = mmap(NULL, sizeof(kv_info), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, sizeof(kv_info)+sizeof(char));
    // printf("does this reach");
    my_sem = sem_open(__KV_WRITERS_SEMAPHORE__,O_CREAT | O_RDWR, S_IRWXU, 1);
    sem_wait(my_sem);
    
    //this is the critical section
    int podIndex = hash(key) % 128;
    pod *actualPod = & (this_kv_info -> pods[podIndex]);
    int actualIndex = (actualPod -> index);

    kv_pair *actualPair = NULL;
    //have to check if key actually exists
    for(i = 0; i < actualIndex; i++){
        kv_pair *currentPair = &(actualPod -> kv_pairs[i]);
        char *storedKey = (currentPair -> key);
        if(strcmp(key, storedKey) == 0){
           keyExists = 1;
           actualPair = & (actualPod -> kv_pairs[i]);
        }
    }

    //if we cant the existing key within the pod; then 
    if(keyExists == 0)
        actualPair = & (actualPod -> kv_pairs[actualIndex]);
  
    int writeIndex = (actualPair -> writeIndex);
    int numberValues = (actualPair -> numberValues);
    if(numberValues <= MAXNUMBERVALUES){
        numberValues++;
        actualPair -> numberValues = numberValues;
    }
    
    memcpy(&actualPair -> key, key, strlen(key));
    memcpy(&actualPair -> value[writeIndex], value , strlen(value) + 1);
    // memcpy(&actualPair -> writeIndex, writeIndex , strlen(writeIndex));
    writeIndex++;
    if(writeIndex >= 8)
        writeIndex = writeIndex % 8;
    // printf("Write index is %d\n", writeIndex);
    actualPair -> writeIndex = writeIndex;

    if(keyExists == 0){
        actualIndex++;
    }
    if(actualIndex > 127){
        actualPod -> index = actualIndex % 128;    
    }
    else{
        actualPod -> index = actualIndex;
    }

    sem_post(my_sem);
    sem_close(my_sem);
    munmap(this_kv_info, sizeof(kv_info));
    close(fd);
    return 0;
}

//once we compute hash and we get to the hash, parse the list of values for the key that we want, by doing key equality
char *kv_store_read(char *key){
    int fd = shm_open(__KV_SHM_NAME__, O_RDWR, S_IRWXU);
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

    my_sem = sem_open(__KV_WRITERS_SEMAPHORE__,O_CREAT | O_RDWR, S_IRWXU, 1);
    sem_wait(my_sem);

    int podIndex = hash(key) % 128;
    pod *actualPod = & (this_kv_info -> pods[podIndex]);
    int numberEntries = (actualPod -> index);
    

    // printf("The pod number is %d\n", podIndex);
    for(i = 0; i < numberEntries; i++){
        kv_pair *currentPair = &(actualPod -> kv_pairs[i]);
        char *storedKey = (currentPair -> key);
        if(strcmp(key, storedKey) == 0){
            int readIndex = (currentPair -> readIndex);
            // printf("Read Index is %d\n", readIndex);
            valueToReturn = strdup((currentPair -> value[readIndex]));
            found = 1;
            readIndex++;
            // printf("NumberValues is %d\n ", currentPair -> numberValues);
            if(readIndex >= currentPair -> numberValues){
                readIndex = readIndex % currentPair -> numberValues;
            }
            currentPair -> readIndex = readIndex;
        }
    }
    sem_post(my_sem);
    sem_close(my_sem);
    munmap(this_kv_info, sizeof(kv_info));
    close(fd);
    
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
    
    int fd = shm_open(__KV_SHM_NAME__, O_RDWR, S_IRWXU);
    if (fd < 0){
       printf("Failed to write to KV \n");
       return NULL;
    }

    this_kv_info = mmap(NULL, sizeof(kv_info), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, sizeof(kv_info) + sizeof(char));

    // printf("does this reach");
    int i,j;
    char *valueToReturn;
    int found = 0;
    int count = 0;
    my_sem = sem_open(__KV_WRITERS_SEMAPHORE__,O_CREAT | O_RDWR, S_IRWXU, 1);
    sem_wait(my_sem);

    int podIndex = hash(key) % 128;
    pod *actualPod = & (this_kv_info -> pods[podIndex]);
    int numberEntries = (actualPod -> index);
    
    //loop to tell us how many values for key
    for(i = 0; i < numberEntries; i++){
        kv_pair *currentPair = &(actualPod -> kv_pairs[i]);
        char *storedKey = (currentPair -> key);
        if(strcmp(key, storedKey) == 0){ 
            for(j = 0; j < currentPair -> numberValues; j++){
                allValues[count] = strdup((currentPair -> value[j]));
                count++;
                found = 1;
            }
        }
    }
    //create 2D char array for the number of values, then loop to add to this 2D char array

    sem_post(my_sem);
    sem_close(my_sem);
    munmap(this_kv_info, sizeof(kv_info));
    close(fd);
    
    if(found == 1){
        return allValues;
    }
    else{
        return NULL;
    }
}
