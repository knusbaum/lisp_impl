#ifndef THREADED_VM_H
#define THREADED_VM_H

#include "object.h"
#include "lisp.h"

void vm_init(context *c);

void compile_bytecode(context *c, object *o);
object *vm_eval(context *c, object *o);

#endif
