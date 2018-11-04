#ifndef _INCLUDE_A2_LIB_H_
#define _INCLUDE_A2_LIB_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <semaphore.h>

#define __TEST_SHARED_MEM_NAME__ "/HQWANG_260688073"
#define __KV_WRITERS_SEMAPHORE__	"/WRITER_HQ_WANG_260688073"
#define NUMBERPODS	128
#define MAXNUMBERVALUES	256

typedef struct kv_pair{
    int readIndex;
    int writeIndex;
    int numberValues;
    char key[32];
    char value[MAXNUMBERVALUES][256];
} kv_pair;

typedef struct pod{
    int index;
    kv_pair kv_pairs[128];
} pod;

typedef struct kv_info{
    char *kv_name;
    pod pods[NUMBERPODS];
} kv_info;

int  kv_store_create(char *name);
int  kv_store_write(char *key,char *value);
char *kv_store_read(char *key);
char **kv_store_read_all(char *key);

#endif