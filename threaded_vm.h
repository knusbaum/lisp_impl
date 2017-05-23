#ifndef THREADED_VM_H
#define THREADED_VM_H

#include "object.h"
#include "context.h"
#include "map.h"

extern map_t *addrs;

void vm_init(context_stack *cs);

void compile_bytecode(compiled_chunk *cc, context_stack *cs, object *o);
object *vm_eval(context_stack *cs, object *o);

map_t *get_vm_addrs();
void run_vm(context_stack *cs, compiled_chunk *cc);

// Should move these.
void push(object *o);
object *pop();
void dump_stack();
void call(context_stack *c, long variance);

#endif
