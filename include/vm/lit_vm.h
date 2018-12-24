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

typedef struct sLitVm {
	LitMemManager mem_manager;

	LitArray globals;
	LitString *init_string;

	LitFiber* fiber;

	LitUpvalue* open_upvalues;
	size_t next_gc;

	int gray_count;
	int gray_capacity;

	LitObject** gray_stack;

	// Std classes
	LitClass* class_class;
	LitClass* object_class;
	LitClass* string_class;
	LitClass* int_class;
	LitClass* double_class;
} sLitVm;

void lit_init_vm(LitVm* vm);
void lit_vm_define_native(LitVm* vm, LitNativeRegistry* native);
void lit_vm_define_natives(LitVm* vm, LitNativeRegistry* natives);

LitClass* lit_vm_define_class(LitVm* vm, LitType* type, LitClass* super);
LitNativeMethod* lit_vm_define_method(LitVm* vm, LitClass* class, LitResolverNativeMethod* method);

typedef struct LitMethodRegistry {
	const char* name;
	const char* signature;
	LitNativeMethodFn fn;
	bool is_static;
} LitMethodRegistry;

typedef struct LitClassRegistry {
	LitType* class;
	LitMethodRegistry* methods;
	LitResolverNativeMethod* natives;
} LitClassRegistry;

typedef struct LitLibRegistry {
	LitClassRegistry** classes;
	LitNativeRegistry** functions;
} LitLibRegistry;

LitClassRegistry* lit_declare_class(LitCompiler* compiler, LitType* type, LitMethodRegistry* methods);
void lit_define_class(LitVm* vm, LitClassRegistry* class);
LitNativeRegistry* lit_declare_native(LitCompiler* compiler, LitNativeFn fn, const char* name, const char* signature);
void lit_define_lib(LitVm* vm, LitLibRegistry* lib);

void lit_free_vm(LitVm* vm);

bool lit_eval(const char* source_code);
bool lit_execute(LitVm* vm, LitFunction* function);

void lit_push(LitVm* vm, LitValue value);

char *lit_to_string(LitVm* vm, LitValue value);

#endif