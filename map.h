#ifndef MAP_H
#define MAP_H

typedef struct map_t map_t;

map_t *map_create();

void map_put(map_t *m, const char *key, void *val);
void *map_get(map_t *m, const char *key);
void *map_delete(map_t *m, char *key);

void map_destroy(map_t *m);

#endif
