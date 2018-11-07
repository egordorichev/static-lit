#ifndef LIT_OBJECT_H
#define LIT_OBJECT_H

#include <lit_common.h>
#include <lit_predefines.h>

#include <vm/lit_value.h>
#include <vm/lit_chunk.h>
#include <util/lit_table.h>
#include <compiler/lit_ast.h>

#define OBJECT_TYPE(value) (AS_OBJECT(value)->type)
#define IS_STRING(value) lit_is_object_type(value, OBJECT_STRING)
#define IS_CLOSURE(value) lit_is_object_type(value, OBJECT_CLOSURE)
#define IS_FUNCTION(value) lit_is_object_type(value, OBJECT_FUNCTION)
#define IS_NATIVE(value) lit_is_object_type(value, OBJECT_NATIVE)
#define IS_METHOD(value) lit_is_object_type(value, OBJECT_BOUND_METHOD)
#define IS_CLASS(value) lit_is_object_type(value, OBJECT_CLASS)
#define IS_INSTANCE(value) lit_is_object_type(value, OBJECT_INSTANCE)

#define AS_CLOSURE(value) ((LitClosure*) AS_OBJECT(value))
#define AS_FUNCTION(value) ((LitFunction*) AS_OBJECT(value))
#define AS_NATIVE(value) (((LitNative*) AS_OBJECT(value))->function)
#define AS_METHOD(value) ((LitMethod*) AS_OBJECT(value))
#define AS_CLASS(value) ((LitClass*) AS_OBJECT(value))
#define AS_INSTANCE(value) ((LitInstance*) AS_OBJECT(value))
#define AS_STRING(value) ((LitString*) AS_OBJECT(value))
#define AS_CSTRING(value) (((LitString*) AS_OBJECT(value))->chars)

typedef enum {
	OBJECT_STRING,
	OBJECT_UPVALUE,
	OBJECT_FUNCTION,
	OBJECT_NATIVE,
	OBJECT_CLOSURE,
	OBJECT_BOUND_METHOD,
	OBJECT_CLASS,
	OBJECT_INSTANCE
} LitObjectType;

struct sLitObject {
	LitObjectType type;
	bool dark;
	struct sLitObject* next;
};

struct sLitString {
	LitObject object;

	int length;
	char* chars;
	uint32_t hash;
};

typedef struct sLitUpvalue {
	LitObject object;

	LitValue* value;
	LitValue closed;
	struct sLitUpvalue* next;
} LitUpvalue;

LitUpvalue* lit_new_upvalue(LitVm* vm, LitValue* slot);

typedef struct {
	LitObject object;

	int arity;
	int upvalue_count;

	LitChunk chunk;
	LitString* name;
} LitFunction;

LitFunction* lit_new_function(LitVm* vm);

typedef int (*LitNativeFn)(LitVm *vm, int count);

typedef struct {
	LitObject object;
	LitNativeFn function;
} LitNative;

LitNative* lit_new_native(LitVm* vm, LitNativeFn function);

typedef struct {
	LitObject object;

	LitFunction* function;
	LitUpvalue** upvalues;
	int upvalue_count;
} LitClosure;

LitClosure* lit_new_closure(LitVm* vm, LitFunction* function);

typedef struct sLitClass {
	LitObject object;

	LitString* name;
	struct sLitClass* super;
	LitTable methods;
	LitTable static_methods;
	LitTable fields;
	LitTable static_fields;
} LitClass;

LitClass* lit_new_class(LitVm* vm, LitString* name, LitClass* super);

typedef struct {
	LitObject object;

	LitClass* type;
	LitTable fields;
} LitInstance;

LitInstance* lit_new_instance(LitVm* vm, LitClass* class);

typedef struct {
	LitObject object;

	LitValue receiver;
	LitClosure* method;
} LitMethod;

LitMethod* lit_new_bound_method(LitVm* vm, LitValue receiver, LitClosure* method);

LitString* lit_make_string(LitVm* vm, char* chars, int length);
LitString* lit_copy_string(LitVm* vm, const char* chars, size_t length);

static inline bool lit_is_object_type(LitValue value, LitObjectType type) {
	return IS_OBJECT(value) && AS_OBJECT(value)->type == type;
}

#endif