#include "hashtable.h"

static uint32_t hash(const char *key) {
    uint32_t hash = 5381;
    int c;
    while ((c = *key++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

hashtable *ht_init(size_t size) {
    hashtable *ht = malloc(sizeof(hashtable));
    ht->size = size;
    ht->buckets = calloc(size, sizeof(ht_entry*));
    ht->num_locks = 16;
    ht->locks = malloc(sizeof(pthread_mutex_t) * ht->num_locks);
    for (size_t i = 0; i < ht->num_locks; i++) {
        pthread_mutex_init(&ht->locks[i], NULL);
    }
    return ht;
}

void ht_set(hashtable *ht, const char *key, void *value) {
    uint32_t h = hash(key);
    size_t idx = h % ht->size;
    size_t lock_idx = idx % ht->num_locks;

    pthread_mutex_lock(&ht->locks[lock_idx]);
    
    ht_entry *entry = ht->buckets[idx];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            free(entry->value);
            entry->value = strdup((char*)value);
            pthread_mutex_unlock(&ht->locks[lock_idx]);
            return;
        }
        entry = entry->next;
    }
    
    entry = malloc(sizeof(ht_entry));
    entry->key = strdup(key);
    entry->value = strdup((char*)value);
    entry->next = ht->buckets[idx];
    ht->buckets[idx] = entry;
    
    pthread_mutex_unlock(&ht->locks[lock_idx]);
}

void *ht_get(hashtable *ht, const char *key) {
    uint32_t h = hash(key);
    size_t idx = h % ht->size;
    size_t lock_idx = idx % ht->num_locks;

    pthread_mutex_lock(&ht->locks[lock_idx]);
    
    ht_entry *entry = ht->buckets[idx];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            void *val = strdup((char*)entry->value);
            pthread_mutex_unlock(&ht->locks[lock_idx]);
            return val;
        }
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&ht->locks[lock_idx]);
    return NULL;
}

void ht_del(hashtable *ht, const char *key) {
    uint32_t h = hash(key);
    size_t idx = h % ht->size;
    size_t lock_idx = idx % ht->num_locks;

    pthread_mutex_lock(&ht->locks[lock_idx]);
    
    ht_entry *entry = ht->buckets[idx];
    ht_entry *prev = NULL;
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                ht->buckets[idx] = entry->next;
            }
            free(entry->key);
            free(entry->value);
            free(entry);
            pthread_mutex_unlock(&ht->locks[lock_idx]);
            return;
        }
        prev = entry;
        entry = entry->next;
    }
    pthread_mutex_unlock(&ht->locks[lock_idx]);
}