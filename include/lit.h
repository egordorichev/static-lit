#ifndef LIT_H
#define LIT_H

#include <compiler/lit_compiler.h>
#include <vm/lit_vm.h>
#include <util/lit_table.h>


LitInterpretResult lit_execute(LitVm* vm, const char* code);
bool lit_interpret(LitVm* vm);
void lit_define_native(LitVm* vm, const char* name, const char* type, LitNativeFn function);

#endif