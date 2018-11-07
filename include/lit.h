#ifndef LIT_H
#define LIT_H

#include <vm/lit_vm.h>
#include <vm/lit_chunk.h>
#include <vm/lit_object.h>

#include <util/lit_table.h>
#include <compiler/lit_resolver.h>

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
	LitString *init_string;

	LitFrame frames[FRAMES_MAX];
	int frame_count;
	bool abort;
	const char* code;

	LitUpvalue* open_upvalues;

	size_t bytes_allocated;
	size_t next_gc;

	int gray_count;
	int gray_capacity;
	LitObject** gray_stack;

	LitResolver resolver;
} _LitVm;

void lit_init_vm(LitVm* vm);
void lit_free_vm(LitVm* vm);

void lit_push(LitVm* vm, LitValue value);
LitValue lit_pop(LitVm* vm);
LitValue lit_peek(LitVm* vm, int depth);

LitInterpretResult lit_execute(LitVm* vm, const char* code);
bool lit_interpret(LitVm* vm);
void lit_define_native(LitVm* vm, const char* name, const char* type, LitNativeFn function);

#endif