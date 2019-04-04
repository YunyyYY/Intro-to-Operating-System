#include "mapreduce.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#define DATASIZE 50
#define KEYSIZE  20

typedef struct __fpointer_t {   // file pointer
    int value;
    pthread_mutex_t lock;
} fpointer_t;

static void init_fpointer(fpointer_t *c) {
    c->value = 0;
    pthread_mutex_init(&c->lock, NULL);
}

typedef struct __data_t {       // store values in key
    char* value;
    struct __data_t* next;      // points to next element, [sorted]
} data_t;

static void freeData(data_t* d) {
    data_t* vim;
    while (d != NULL) {
        free(d->value);
        vim = d;
        d = d->next;
        free(vim);
    }
}

typedef struct __box_t {        // box [key, values]
    char* key;
    data_t* data;               // points to the data list
    size_t size;                // number of values in this box
    struct __box_t* next;       // points to next box, [sorted]
    pthread_mutex_t lock;
    data_t* tmp;                // used during reduce for current
} box_t;

static void init_box(box_t *b, char* key) {
    b->key = malloc(KEYSIZE * sizeof(char));
    strcpy(b->key, key);
    b->data = NULL;
    b->size = 0;
    b->next = NULL;
    pthread_mutex_init(&b->lock, NULL);
}

static void freeBox(box_t *b) {
    box_t* vim;
    while(b != NULL) {
        free(b->key);
        freeData(b->data);
        vim = b;
        b = b->next;
        free(vim);
        //  printf("62| freeBox\n");
    }

}

typedef struct __unit_t {       // unit, each is a partition
    box_t* box;                 // [root] of box list for each partition
    size_t size;                // number of boxes in this unit
    pthread_mutex_t lock;
    box_t* tmp;
} unit_t;

static void init_unit(unit_t *u) {
    u->size = 0;
    u->box = NULL;
    pthread_mutex_init(&u->lock, NULL);
}

static void freeUnit(unit_t* u) {
    freeBox(u->box);        // printf("82| freeUnit\n");
}

static box_t* getKey(unit_t *u, char* key) {        //  if key not exist,
    pthread_mutex_lock(&u->lock);                   //      create and return
    box_t * tmp = u->box;
    box_t * prev = u->box;
    while(tmp != NULL && strcmp(tmp->key, key)<0){
        prev = tmp;
        tmp = tmp->next;
    }
    if (tmp == NULL || strcmp(tmp->key, key) > 0) {
        box_t * new = malloc(sizeof(box_t));
        init_box(new, key);
        if (prev == NULL)            // first key
            tmp = u->box = new;
        else {
            if (tmp == u->box)          // insert in the front
                u->box = new;
            else
                prev->next = new;
            new->next = tmp;
            tmp = new;
        }
        u->size += 1;
    }
    pthread_mutex_unlock(&u->lock);
    return tmp;
}

static void addData(box_t *b, char* value) {
    pthread_mutex_lock(&b->lock);
    data_t * tmp = b->data;
    data_t * prev = b->data;
    while(tmp != NULL && strcmp(tmp->value, value) < 0){
        prev = tmp;
        tmp = tmp->next;
    }
    data_t *new = malloc(sizeof(data_t));
    new->value = malloc(DATASIZE * sizeof(char));
    strcpy(new->value, value);
    new->next = tmp;
    if (prev == NULL)           // first data
        b->data = new;
    else {
        if (tmp == b->data)         // insert in the front
            b->data = new;
        else
            prev->next = new;
    }
    b->size += 1;
    pthread_mutex_unlock(&b->lock);
}

// intermediate data strcutures section
static int total_files;
static char** files;        // store the address of file names (argv)
static unit_t* unit;        // store the address of intermediate data structure
static int units;

static fpointer_t current;  // stores current file to be read
static fpointer_t curunit;  // stores current unit
static Partitioner getUnit;

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

static void setIterBox(box_t* b) {
    b->tmp = b->data;
}

static void setIterUnit(unit_t* u) {
    u->tmp = u->box;
    box_t* b = u->box;
    while (b != NULL) {
        setIterBox(b);
        b = b->next;
    }
}

char* getNext(char* key, int index) {  // the data returned is  dynamic!
    box_t *b = getKey(&unit[index], key);
    if (b->tmp == NULL)
        return NULL;
    char* data  = b->tmp->value;
    b->tmp = b->tmp->next;
    return data;
}

static void *mySum(void *arg) {
    Reducer reduce = (Reducer) arg;
    box_t *b;
    int i;
    int idx, par;
    while (1){
        pthread_mutex_lock(&curunit.lock);
        idx = curunit.value;
        curunit.value = (idx + 1) % units;
        pthread_mutex_unlock(&curunit.lock);
        b = NULL;  // every round reset box

        for (i = 0; i < units; i++) {
            par = (idx + i) % units;
            pthread_mutex_lock(&unit[par].lock);
            b = unit[par].tmp;
            if(b != NULL) {
                unit[par].tmp = unit[par].tmp->next;
                pthread_mutex_unlock(&unit[par].lock);
                break;
            }
            pthread_mutex_unlock(&unit[par].lock);
        }
//        if (par == -1)
//            return NULL;
        if (b == NULL)
            return NULL;
        reduce(b->key, getNext, par);
    }
}

void MR_Emit(char *key, char *value) {
    unsigned long index = getUnit(key, units);

    box_t* b = getKey(&unit[index], key);
    addData(b, value); // printf("214| %lu %s\n", index, key);

    // need data structure to store intermediate values
    // can assume they fits in memory
    // how multiple map threads are going to insert data into this data structure
    // how do we get all the values for a particular key to the same reducer

    // reducer need to wait for mappers
    // -- getter functions needs all values for a key in the reducer
    // -- reducer needs to see all keys in order

}

unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
    unsigned long hash = 5381;
    int c;    // printf("229-232| %s ", key);
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    // printf(" %lu \n", hash);
    return hash % num_partitions;
}

void MR_Run(int argc, char *argv[],
            Mapper map, int num_mappers,
            Reducer reduce, int num_reducers,
            Partitioner partition) {

    init_fpointer(&current);
    init_fpointer(&curunit);

    // [initialize]
    unit = malloc(num_reducers * sizeof(unit_t));
    for (int i = 0; i < num_reducers; i++) {
        init_unit(&unit[i]);
    }
    pthread_t * p_map = (pthread_t *)malloc(sizeof(pthread_t) * num_mappers);
    pthread_t* p_red = malloc(sizeof(pthread_t) * num_reducers);
    getUnit = partition;

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
        setIterUnit(&unit[i]);
    }

    // [reduce]
    for (int i = 0; i < num_reducers; i++) {
        rc = pthread_create(&p_red[i], NULL, mySum, reduce);
        assert(rc == 0);
    }

    // [wait] for all mappers to finish
    for (int i = 0; i < num_reducers; i++) {
        rc = pthread_join(p_red[i], NULL);
        assert(rc == 0);
    }

    // [free] memory
    for (int i = 0; i < num_reducers; i++) {
        freeUnit(&unit[i]);
    }
    free(unit);
    free(p_map);
    free(p_red);
}