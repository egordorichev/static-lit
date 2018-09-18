#ifndef LIT_H
#define LIT_H

#include "lit_vm.h"
#include "lit_chunk.h"
#include "lit_compiler.h"

typedef struct _LitVm {
	LitValue stack[STACK_MAX];
	LitValue* stack_top;
	LitChunk* chunk;

	uint8_t* ip;
	LitCompiler *compiler;
};

void lit_init_vm(LitVm* vm);
void lit_free_vm(LitVm* vm);

void lit_push(LitVm* vm, LitValue value);
LitValue lit_pop(LitVm* vm);
LitValue lit_peek(LitVm* vm, int depth);

LitInterpretResult lit_execute(LitVm* vm, const char* code);
LitInterpretResult lit_interpret(LitVm* vm, LitChunk* chunk);

#endif