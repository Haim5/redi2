#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "common.h"
#include <pthread.h>

typedef struct ht_entry {
    char *key;
    void *value;
    struct ht_entry *next;
} ht_entry;

typedef struct {
    ht_entry **buckets;
    size_t size;
    pthread_mutex_t *locks;
    size_t num_locks;
} hashtable;

hashtable *ht_init(size_t size);
void ht_set(hashtable *ht, const char *key, void *value);
void *ht_get(hashtable *ht, const char *key);
void ht_del(hashtable *ht, const char *key);

#endif