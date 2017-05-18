#ifndef MAP_H
#define MAP_H

/**
 * Map does not own the keys or values, and will not 
 * attempt to copy, modify, or free them. Keys and values 
 * must be kept alive as long as they are held in the map,
 * or until map_destroy is called.
 */

typedef struct map_t map_t;

map_t *map_create(int (*equal)(void *x, void *y));

void map_put(map_t *m, void *key, void *val);
void *map_get(map_t *m, void *key);
void *map_delete(map_t *m, void *key);

void map_destroy(map_t *m);

#endif
