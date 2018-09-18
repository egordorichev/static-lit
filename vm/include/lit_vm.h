#ifndef LIT_VM_H
#define LIT_VM_H

#include "lit_common.h"
#include "lit_value.h"

#define STACK_MAX 256

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
} LitInterpretResult;

typedef struct {
	LitValue stack[STACK_MAX];
	LitValue* stackTop;

	struct LitCompiler *compiler;
} LitVm;

void lit_init_vm(LitVm* vm);
void lit_free_vm(LitVm* vm);

LitInterpretResult lit_execute(LitVm* vm, const char* code);

#endif