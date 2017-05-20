#ifndef COMPILER_H
#define COMIILER_H

#include "object.h"
#include "map.h"
#include "context.h"

struct binstr {
    void *instr;
    union {
        object *arg;
        long variance;
        char *str;
        size_t offset;
    };
};

typedef struct compiled_chunk {
    struct binstr *bs;
    size_t b_off;
    size_t bsize;
    map_t *labels;
} compiled_chunk;


void compiler_init();
compiled_chunk *compile_form(context *c, object *o);

#endif
