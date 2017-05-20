#ifndef THREADED_VM_H
#define THREADED_VM_H

#include "object.h"
#include "lisp.h"

//typedef struct binstr binstr;
typedef struct compiled_chunk compiled_chunk;

void vm_init(context *c);

void compile_bytecode(compiled_chunk *cc, context *c, object *o);
object *vm_eval(context *c, object *o);


// Should move these.
void push(object *o);
object *pop();
void dump_stack();
void call(context *c, long variance);

#endif
