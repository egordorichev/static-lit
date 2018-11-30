#ifndef LIT_VM_H
#define LIT_VM_H

/*
 * The lit VM
 * Runs bytecode chunks
 */

#include <lit_common.h>
#include <lit_predefines.h>
#include <lit_mem_manager.h>

#include <vm/lit_chunk.h>
#include <vm/lit_object.h>
#include <vm/lit_memory.h>
#include <compiler/lit_resolver.h>

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
	LitTable globals;
	LitString *init_string;

	LitFrame frames[FRAMES_MAX];
	int frame_count;
	bool abort;

	LitUpvalue* open_upvalues;
	size_t next_gc;

	int gray_count;
	int gray_capacity;

	LitObject** gray_stack;
} sLitVm;

void lit_init_vm(LitVm* vm);
void lit_vm_define_native(LitVm* vm, LitNativeRegistry* native);
void lit_vm_define_natives(LitVm* vm, LitNativeRegistry* natives);

LitClass* lit_vm_define_class(LitVm* vm, LitType* type, LitClass* super);

void lit_free_vm(LitVm* vm);

bool lit_eval(const char* source_code);
bool lit_execute(LitVm* vm, LitFunction* function);

void lit_push(LitVm* vm, LitValue value);
LitValue lit_pop(LitVm* vm);
LitValue lit_peek(LitVm* vm, int depth);

char *lit_to_string(LitVm* vm, LitValue value);

#endif