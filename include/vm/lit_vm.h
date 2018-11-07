#ifndef LIT_VM_H
#define LIT_VM_H

#include <lit_common.h>
#include <lit_predefines.h>
#include <vm/lit_chunk.h>
#include <vm/lit_object.h>
#include <vm/lit_memory.h>

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
} LitInterpretResult;

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)
#define VM_STACK_MAX 256

typedef struct {
	LitClosure* closure;
	uint8_t* ip;
	LitValue* slots;
} LitFrame;

typedef struct sLitVm {
	LitMemManager mem_manager;

	LitValue stack[VM_STACK_MAX];
	LitValue* stack_top;
	LitObject* objects;
	LitTable strings;
	LitTable globals;
	LitString *init_string;

	LitFrame frames[FRAMES_MAX];
	int frame_count;
	bool abort;
	const char* code;

	LitUpvalue* open_upvalues;
	size_t next_gc;

	int gray_count;
	int gray_capacity;

	LitObject** gray_stack;
} sLitVm;

void lit_init_vm(LitVm* vm);
void lit_free_vm(LitVm* vm);

void lit_push(LitVm* vm, LitValue value);
LitValue lit_pop(LitVm* vm);
LitValue lit_peek(LitVm* vm, int depth);

char *lit_to_string(LitVm* vm, LitValue value);

#endif