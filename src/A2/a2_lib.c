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

//hash function that is given, compute a hash of a string
unsigned long hash(unsigned char *str){
    unsigned long hash = 0;
    int c;

    while(c = *str++)
        hash = c + (hash << 6) + (hash << 16) - hash;
    return (hash > 0) ? hash : -(hash);
}

//method to create the kv_store
int kv_store_create(char *name){
    // create the kv at the designated name
    int fd = shm_open(__KV_SHM_NAME__, O_CREAT | O_RDWR, S_IRWXU);
    
    if (fd < 0){
       printf("Failed to create a KV");
       return -1;
    }
    shmname = name;
    //map the location of the shared memory
    this_kv_info = mmap(NULL, sizeof(kv_info), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, sizeof(kv_info));
    this_kv_info -> kv_name;
    // after doing stuff dereference the shared memory
    munmap(this_kv_info, sizeof(kv_info));
    close(fd);
    return 0;
}


// method to write to the kv_store
int kv_store_write(char *key, char *value){

    // truncate the string with a null value to avoid issues later on
    if(strlen(key) > 32){
        *(key + 32) = '\0';
    }
    if(strlen(value) > 256){
        *(value + 256) = '\0';
    }

    int i;
    int keyExists = 0;
    // open the shared memory
    int fd = shm_open(__KV_SHM_NAME__, O_RDWR, S_IRWXU);
    if (fd < 0){
       printf("Failed to write to KV \n");
       return -1;
    }

    // reference the shared memory
    this_kv_info = mmap(NULL, sizeof(kv_info), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, sizeof(kv_info)+sizeof(char));
    
    // set up the write semaphore and obtain a lock 
    my_sem_write = sem_open(__KV_WRITERS_SEMAPHORE__,O_CREAT | O_RDWR, S_IRWXU, 1);
    sem_wait(my_sem_write);
    
    // this is the start of critical section
    // identify which pod that  contains the kv-pair
    int podIndex = hash(key) % NUMBERPODS;
    pod *actualPod = & (this_kv_info -> pods[podIndex]);
    int actualIndex = (actualPod -> index);

    kv_pair *actualPair = NULL;
    // have to check if key already exists inside this pod
    for(i = 0; i < actualIndex; i++){
        kv_pair *currentPair = &(actualPod -> kv_pairs[i]);
        char *storedKey = (currentPair -> key);
        if(strcmp(key, storedKey) == 0){
           keyExists = 1;
           actualPair = & (actualPod -> kv_pairs[i]);
        }
    }

    // if we cant find the existing key within the pod
    // create a new kv-pair inside the pod
    if(keyExists == 0)
        actualPair = & (actualPod -> kv_pairs[actualIndex]);
    // figure out which position to write to based on the writeIndex
    int writeIndex = (actualPair -> writeIndex);
    int numberValues = (actualPair -> numberValues);

    //if number of values is too high, use FIFO
    if(numberValues <= MAXNUMBERVALUES){
        numberValues++;
        actualPair -> numberValues = numberValues;
    }
    
    //write to the position in memory
    memcpy(&actualPair -> key, key, strlen(key));
    memcpy(&actualPair -> value[writeIndex], value , strlen(value) + 1);

    //increment the writeindex
    writeIndex++;
    if(writeIndex >= MAXNUMBERVALUES)
        writeIndex = writeIndex % MAXNUMBERVALUES;

    actualPair -> writeIndex = writeIndex;

    //if create a new key, then increment number of kv-pairs
    if(keyExists == 0){
        actualIndex++;
    }
    if(actualIndex >= NUMBERKVPAIR){
        actualPod -> index = actualIndex % NUMBERKVPAIR;    
    }
    else{
        actualPod -> index = actualIndex;
    }

    //release the semaphore and unlink semaphore/shared memory
    sem_post(my_sem_write);
    sem_close(my_sem_write); 
    munmap(this_kv_info, sizeof(kv_info));
    close(fd);
    return 0;
}

//once we compute hash and we get to the hash, parse the list of values for the key that we want, by doing key equality
char *kv_store_read(char *key){
    //open the shared memory
    int fd = shm_open(__KV_SHM_NAME__, O_RDWR, S_IRWXU);
    if (fd < 0){
       printf("Failed to write to KV \n");
       return NULL;
    }
    //map the location of shared mem
    this_kv_info = mmap(NULL, sizeof(kv_info), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ftruncate(fd, sizeof(kv_info) + sizeof(char));
    int i;
    char *valueToReturn;
    int found = 0;
    //obtain lock on read
    my_sem_read = sem_open(__KV_READERS_SEMAPHORE__,O_CREAT | O_RDWR, S_IRWXU, 1);
    my_sem_write = sem_open(__KV_WRITERS_SEMAPHORE__,O_CREAT | O_RDWR, S_IRWXU, 1);
    sem_wait(my_sem_read);
    //if we can read, get lock on write
    (this_kv_info -> readCount)++;
    if((this_kv_info -> readCount) == 1){
        sem_wait(my_sem_write);
    }
    sem_post(my_sem_read);

    //figure out which pod to read from
    int podIndex = hash(key) % NUMBERPODS;
    pod *actualPod = & (this_kv_info -> pods[podIndex]);
    int numberEntries = (actualPod -> index);
    
    //check every kv-pair to see which key we can read
    for(i = 0; i < numberEntries; i++){
        kv_pair *currentPair = &(actualPod -> kv_pairs[i]);
        char *storedKey = (currentPair -> key);
        if(strcmp(key, storedKey) == 0){
            //based on the readIndex, read the approriate value
            int readIndex = (currentPair -> readIndex);
            //duplicate that string
            valueToReturn = strdup((currentPair -> value[readIndex]));
            found = 1;
            readIndex++;
            if(readIndex >= currentPair -> numberValues){
                readIndex = readIndex % currentPair -> numberValues;
            }
            currentPair -> readIndex = readIndex;
        }
    }
    //get a lock on read
    sem_wait(my_sem_read);
    (this_kv_info -> readCount)--;
    
    //release the write lock
    if((this_kv_info -> readCount) == 0){
        sem_post(my_sem_write);
    }
    //dereference everything
    sem_post(my_sem_read);
    sem_close(my_sem_read);
    sem_close(my_sem_write);

    munmap(this_kv_info, sizeof(kv_info));
    close(fd);
    //if found value, return; else return NULL
    if(found == 1){
        return valueToReturn;
    }
    else{
        return NULL;
    }
}
// read all values of a specific key
char ** kv_store_read_all(char *key){
    //allocate enough memory for the array of pointers
	char** allValues = calloc((MAXNUMBERVALUES + 1), sizeof(char*));
    //link to shared memory
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
    //get a lock on read so that we can get a lock on write
    my_sem_read = sem_open(__KV_READERS_SEMAPHORE__,O_CREAT | O_RDWR, S_IRWXU, 1);
    my_sem_write = sem_open(__KV_WRITERS_SEMAPHORE__,O_CREAT | O_RDWR, S_IRWXU, 1);
    sem_wait(my_sem_read);

    (this_kv_info -> readCount)++;
    if((this_kv_info -> readCount) == 1){
        sem_wait(my_sem_write);
    }
    sem_post(my_sem_read);

    //figure out which pod to read from
    int podIndex = hash(key) % NUMBERPODS;
    pod *actualPod = & (this_kv_info -> pods[podIndex]);
    int numberEntries = (actualPod -> index);
    
    //find if key matches which existing ones
    for(i = 0; i < numberEntries; i++){
        kv_pair *currentPair = &(actualPod -> kv_pairs[i]);
        char *storedKey = (currentPair -> key);
        if(strcmp(key, storedKey) == 0){
            //if found, read all values and duplicate into the array of chars
            for(j = 0; j < (currentPair -> numberValues); j++){
                allValues[j] = strdup((currentPair -> value[j]));
            }
            found = 1;
            break;
        }
    }
    //release lock on read and lock on write
    sem_wait(my_sem_read);
    (this_kv_info -> readCount)--;
    
    if((this_kv_info -> readCount) == 0){
        sem_post(my_sem_write);
    }
    //dereference everything
    sem_post(my_sem_read);
    sem_close(my_sem_read);
    sem_close(my_sem_write);

    munmap(this_kv_info, sizeof(kv_info));
    close(fd);
    //return if found
    if(found == 1){
        return allValues;
    }
    else{
        return NULL;
    }
}
