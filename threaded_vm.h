#ifndef THREADED_VM_H
#define THREADED_VM_H

#include "object.h"
#include "lisp.h"
#include "map.h"

extern map_t *addrs;

void vm_init(context *c);

void compile_bytecode(compiled_chunk *cc, context *c, object *o);
object *vm_eval(context *c, object *o);


// Should move these.
void push(object *o);
object *pop();
void dump_stack();
void call(context *c, long variance);

#endif
