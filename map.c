#include <stdlib.h>
#include <string.h>
#include "map.h"

typedef struct map_t {
    void **keys;
    void **vals;
    size_t count;
    size_t space;
    int (*equal)(void *x, void *y);
} map_t;

void increase_map_space(map_t *m) {
    m->space *= 2;
    m->keys = realloc(m->keys, sizeof(char **) * m->space);
    m->vals = realloc(m->vals, sizeof(void **) * m->space);
}

#define INITIAL_SPACE 8
map_t *map_create(int (*equal)(void *x, void *y)) {
    map_t *m = malloc(sizeof (map_t));
    m->keys = malloc(sizeof (char **) * INITIAL_SPACE);
    m->vals = malloc(sizeof (void **) * INITIAL_SPACE);
    m->count = 0;
    m->space = INITIAL_SPACE;
    m->equal = equal;
    return m;
}

void map_put(map_t *m, void *key, void *val) {
    if(m->count == m->space) {
        increase_map_space(m);
    }
    for(size_t i = 0; i < m->count; i++) {
        if(m->equal(m->keys[i], key)) {
            m->vals[i] = val;
            return;
        }
    }
    m->keys[m->count] = key;
    m->vals[m->count] = val;
    m->count++;
}

void *map_get(map_t *m, void *key) {
    if(key == NULL) return NULL;
    
    for(size_t i = 0; i < m->count; i++) {
        if(m->equal(m->keys[i], key)) {
            return m->vals[i];
        }
    }
    return NULL;
}

void *map_delete(map_t *m, void *key) {
    for(size_t i = 0; i < m->count; i++) {
        if(m->equal(m->keys[i], key)) {
            void *ret = m->vals[i];
            m->count--;
            m->keys[i] = m->keys[m->count];
            m->vals[i] = m->vals[m->count];
            return ret;
        }
    }
    return NULL;
}

void map_destroy(map_t *m) {
    free(m->keys);
    free(m->vals);
    free(m);
}
