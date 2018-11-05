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
sem_t *my_sem_write;
sem_t *my_sem_read;

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

    if(strlen(key) > 32){
        *(key + 32) = '\0';
    }
    if(strlen(value) > 256){
        *(value + 256) = '\0';
    }

    int i;
    int keyExists = 0;
    int fd = shm_open(__KV_SHM_NAME__, O_RDWR, S_IRWXU);
    if (fd < 0){
       printf("Failed to write to KV \n");
       return -1;
    }

    this_kv_info = mmap(NULL, sizeof(kv_info), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, sizeof(kv_info)+sizeof(char));

    my_sem_write = sem_open(__KV_WRITERS_SEMAPHORE__,O_CREAT | O_RDWR, S_IRWXU, 1);
    sem_wait(my_sem_write);
    
    //this is the critical section
    int podIndex = hash(key) % NUMBERPODS;
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

    writeIndex++;
    if(writeIndex >= MAXNUMBERVALUES)
        writeIndex = writeIndex % MAXNUMBERVALUES;

    actualPair -> writeIndex = writeIndex;

    if(keyExists == 0){
        actualIndex++;
    }
    if(actualIndex >= NUMBERKVPAIR){
        actualPod -> index = actualIndex % NUMBERKVPAIR;    
    }
    else{
        actualPod -> index = actualIndex;
    }

    sem_post(my_sem_write);
    sem_close(my_sem_write);
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
    int i;
    char *valueToReturn;
    int found = 0;

    my_sem_read = sem_open(__KV_READERS_SEMAPHORE__,O_CREAT | O_RDWR, S_IRWXU, 1);
    sem_wait(my_sem_read);

    int podIndex = hash(key) % NUMBERPODS;
    pod *actualPod = & (this_kv_info -> pods[podIndex]);
    int numberEntries = (actualPod -> index);
    
    for(i = 0; i < numberEntries; i++){
        kv_pair *currentPair = &(actualPod -> kv_pairs[i]);
        char *storedKey = (currentPair -> key);
        if(strcmp(key, storedKey) == 0){
            int readIndex = (currentPair -> readIndex);
            valueToReturn = strdup((currentPair -> value[readIndex]));
            found = 1;
            readIndex++;
            if(readIndex >= currentPair -> numberValues){
                readIndex = readIndex % currentPair -> numberValues;
            }
            currentPair -> readIndex = readIndex;
        }
    }
    sem_post(my_sem_read);
    sem_close(my_sem_read);
    munmap(this_kv_info, sizeof(kv_info));
    close(fd);
    
    if(found == 1){
        return valueToReturn;
    }
    else{
        return NULL;
    }
}

char ** kv_store_read_all(char *key){
	char** allValues = calloc((MAXNUMBERVALUES + 1), sizeof(char*));

	int fd = shm_open(__KV_SHM_NAME__, O_RDWR, S_IRWXU);
    if (fd < 0){
       printf("Failed to write to KV \n");
       return NULL;
    }

    this_kv_info = mmap(NULL, sizeof(kv_info), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, sizeof(kv_info) + sizeof(char));
    int i;
    int j;
    int found = 0;

    my_sem_read = sem_open(__KV_READERS_SEMAPHORE__,O_CREAT | O_RDWR, S_IRWXU, 1);
    sem_wait(my_sem_read);

    int podIndex = hash(key) % NUMBERPODS;
    pod *actualPod = & (this_kv_info -> pods[podIndex]);
    int numberEntries = (actualPod -> index);
    
    for(i = 0; i < numberEntries; i++){
        kv_pair *currentPair = &(actualPod -> kv_pairs[i]);
        char *storedKey = (currentPair -> key);
        if(strcmp(key, storedKey) == 0){
            for(j = 0; j < (currentPair -> numberValues); j++){
                allValues[j] = strdup((currentPair -> value[j]));
            }
            found = 1;
            break;
        }
    }
    sem_post(my_sem_read);
    sem_close(my_sem_read);
    munmap(this_kv_info, sizeof(kv_info));
    close(fd);
    
    if(found == 1){
        return allValues;
    }
    else{
        return NULL;
    }
}
