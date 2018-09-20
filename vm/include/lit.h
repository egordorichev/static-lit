#ifndef LIT_H
#define LIT_H

#include "lit_vm.h"
#include "lit_chunk.h"
#include "lit_compiler.h"
#include "lit_table.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)
#define VM_STACK_MAX 256

typedef struct {
	LitClosure* closure;
	uint8_t* ip;
	LitValue* slots;
} LitFrame;

typedef struct _LitVm {
	LitValue stack[VM_STACK_MAX];
	LitValue* stack_top;
	LitObject* objects;
	LitTable strings;
	LitTable globals;

	LitFrame frames[FRAMES_MAX];
	int frame_count;

	LitUpvalue* open_upvalues;

	LitCompiler *compiler;

	size_t bytes_allocated;
	size_t next_gc;

	int gray_count;
	int gray_capacity;
	LitObject** gray_stack;
} _LitVm;

void lit_init_vm(LitVm* vm);
void lit_free_vm(LitVm* vm);

void lit_push(LitVm* vm, LitValue value);
LitValue lit_pop(LitVm* vm);
LitValue lit_peek(LitVm* vm, int depth);

LitInterpretResult lit_execute(LitVm* vm, const char* code);
LitInterpretResult lit_interpret(LitVm* vm);

#endif