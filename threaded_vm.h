#ifndef THREADED_VM_H
#define THREADED_VM_H

#include "object.h"
#include "lisp.h"

void compile_bytecode(context *c, object *o);
object *vm_eval(object *o);

#endif
