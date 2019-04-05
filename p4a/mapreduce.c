#include "mapreduce.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#define BOXSIZE 100
#define KEYSIZE  25
#define VALUESIZE 25

typedef struct __fpointer_t {   // file pointer
    int value;
    pthread_mutex_t lock;
} fpointer_t;

static void init_fpointer(fpointer_t *c) {
    c->value = 0;
    pthread_mutex_init(&c->lock, NULL);
}

typedef struct __pair_t {
    char key[KEYSIZE];
    char value[VALUESIZE];
} pair_t;

typedef struct __unit_t {        // unit, each is a partition
    pair_t* box;                 // [root] of box list for each partition
    int tmp;                 // try to remember current char*
    int size;
    int capacity;
    pthread_mutex_t lock;
} unit_t;

static void init_unit(unit_t *u) {
    u->box = calloc(BOXSIZE, sizeof(pair_t));
    u->tmp = 0;
    u->size = 0;
    u->capacity = BOXSIZE;
    pthread_mutex_init(&u->lock, NULL);
}

static void addData(unit_t *u, char* key, char* value) {
    pthread_mutex_lock(&u->lock);

    if (u->size == u->capacity) {
        u->capacity *= 2;
        u->box = realloc(u->box, sizeof(pair_t) * u->capacity);
    }
    strcpy(u->box[u->size].key, key);
    strcpy(u->box[u->size].value, value);
    u->size += 1;

    pthread_mutex_unlock(&u->lock);
}

// intermediate data strcutures section
static int total_files;
static char** files;        // store the address of file names (argv)
static unit_t* unit;        // store the address of intermediate data structure
static int units;

static fpointer_t current;  // stores current file to be read
static Partitioner getUnit;
static Reducer myReduce;

static void *myRead(void *arg) {
    int file_num;

    Mapper map = (Mapper) arg;
    while(1) {
        pthread_mutex_lock(&current.lock);
        file_num  = current.value;
        current.value++;
        pthread_mutex_unlock(&current.lock);
        if(file_num >= total_files)
            break;
        map(files[file_num]);
    }

    return NULL;
}

char* getNext(char* key, int index) {  // the data returned is dynamic!
    int tmp = unit[index].tmp;
    if (tmp >= unit[index].size)
        return NULL;
    pair_t *p = &unit[index].box[tmp];
    if (strcmp(key, p->key) == 0) {
        unit[index].tmp += 1;
        return p->value;
    }
    else return NULL;
}

int myCompare(const void *a, const void *b) {
    const pair_t* s1 = (pair_t* )a;
    const pair_t* s2 = (pair_t* )b;
    if (strcmp(s1->key, s2->key)==0)
        return strcmp(s1->value, s2->value);
    return strcmp(s1->key, s2->key);
}

static void *mySort(void *arg) {
    unit_t *u = (unit_t *)arg;
    qsort(u->box, (size_t )u->size, sizeof(pair_t), myCompare);
    return NULL;
}

static void *mySum(void *arg) {
    int idx = *(int *)arg;
    pair_t *p = unit[idx].box;
    int size = unit[idx].size;
    int tmp = 0;
    while (tmp < size) {
        char* key = p[tmp].key;
        myReduce(key, getNext, idx);
        tmp = unit[idx].tmp;
    }
    return NULL;
}

void MR_Emit(char *key, char *value) {
    unsigned long index = getUnit(key, units);
    addData(&unit[index], key, value); // printf("214| %lu %s\n", index, key);

}

unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;

    return hash % num_partitions;
}

void MR_Run(int argc, char *argv[],
            Mapper map, int num_mappers,
            Reducer reduce, int num_reducers,
            Partitioner partition) {

    init_fpointer(&current);

    // [initialize]
    unit = malloc(num_reducers * sizeof(unit_t));
    for (int i = 0; i < num_reducers; i++) {
        init_unit(&unit[i]);
    }
    pthread_t * p_map = (pthread_t *)malloc(sizeof(pthread_t) * num_mappers);
    pthread_t* p_red = malloc(sizeof(pthread_t) * num_reducers);
    getUnit = partition;
    myReduce = reduce;

    // [parse] the input
    assert(argc > 1);
    int rc;
    total_files = argc;  // file number start at 1, #max < argc
    current.value = 1;
    files = argv;
    units = num_reducers;

    // [start routine]
    for (int i = 0; i < num_mappers; i++) {
        rc = pthread_create(&p_map[i], NULL, myRead, map);
        assert(rc == 0);
    }

    // [wait] for all mappers to finish
    for (int i = 0; i < num_mappers; i++) {
        rc = pthread_join(p_map[i], NULL);
        assert(rc == 0);
    }

    // [sort] they are already sorted... Just initialize iterator
    for (int i = 0; i < num_reducers; i++) {
        rc = pthread_create(&p_red[i], NULL, mySort, &unit[i]);
        assert(rc == 0);
    }
    for (int i = 0; i < num_reducers; i++) {
        rc = pthread_join(p_red[i], NULL);
        assert(rc == 0);
    }

    // [reduce]
    int* t = malloc(sizeof(int) * num_reducers);
    for (int i = 0; i < num_reducers; i++) {
        t[i] = i;
        rc = pthread_create(&p_red[i], NULL, mySum, &t[i]);
        assert(rc == 0);
    }

    // [wait] for all mappers to finish
    for (int i = 0; i < num_reducers; i++) {
        rc = pthread_join(p_red[i], NULL);
        assert(rc == 0);
    }


    // [free] memory
    for (int i = 0; i < num_reducers; i++) {
        free(unit[i].box);
    }
    free(unit);
    free(p_map);
    free(p_red);
    free(t);
}