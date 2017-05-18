#include <stdlib.h>
#include <string.h>
#include "map.h"

typedef struct map_t {
    char **keys;
    void **vals;
    size_t count;
    size_t space;
} map_t;

void increase_map_space(map_t *m) {
    m->space *= 2;
    m->keys = realloc(m->keys, sizeof(char **) * m->space);
    m->vals = realloc(m->vals, sizeof(void **) * m->space);
}

#define INITIAL_SPACE 8
map_t *map_create() {
    map_t *m = malloc(sizeof (map_t));
    m->keys = malloc(sizeof (char **) * INITIAL_SPACE);
    m->vals = malloc(sizeof (void **) * INITIAL_SPACE);
    m->count = 0;
    m->space = INITIAL_SPACE;
    return m;
}

void map_put(map_t *m, const char *key, void *val) {
    if(m->count == m->space) {
        increase_map_space(m);
    }
    for(size_t i = 0; i < m->count; i++) {
        if(strcmp(m->keys[i], key) == 0) {
            m->vals[i] = val;
            return;
        }
    }
    m->keys[m->count] = strdup(key);
    m->vals[m->count] = val;
    m->count++;
}

void *map_get(map_t *m, const char *key) {
    if(key == NULL) return NULL;
    
    for(size_t i = 0; i < m->count; i++) {
        if(strcmp(m->keys[i], key) == 0) {
            return m->vals[i];
        }
    }
    return NULL;
}

void *map_delete(map_t *m, char *key) {
    for(size_t i = 0; i < m->count; i++) {
        if(strcmp(m->keys[i], key) == 0) {
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
