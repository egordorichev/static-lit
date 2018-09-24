#ifndef LIT_OBJECT_H
#define LIT_OBJECT_H

#include "lit_common.h"
#include "lit_value.h"
#include "lit_vm.h"
#include "lit_chunk.h"

#define OBJECT_TYPE(value) (AS_OBJECT(value)->type)
#define IS_STRING(value) lit_is_object_type(value, OBJECT_STRING)
#define IS_CLOSURE(value) lit_is_object_type(value, OBJECT_CLOSURE)
#define IS_FUNCTION(value) lit_is_object_type(value, OBJECT_FUNCTION)
#define IS_NATIVE(value) lit_is_object_type(value, OBJECT_NATIVE)

#define AS_CLOSURE(value) ((LitClosure*) AS_OBJECT(value))
#define AS_FUNCTION(value) ((LitFunction*) AS_OBJECT(value))
#define AS_NATIVE(value) (((LitNative*) AS_OBJECT(value))->function)
#define AS_STRING(value) ((LitString*) AS_OBJECT(value))
#define AS_CSTRING(value) (((LitString*) AS_OBJECT(value))->chars)

typedef enum {
	OBJECT_STRING,
	OBJECT_UPVALUE,
	OBJECT_FUNCTION,
	OBJECT_NATIVE,
	OBJECT_CLOSURE,
	OBJ_BOUND_METHOD,
	OBJ_CLASS,
	OBJ_INSTANCE
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

typedef struct {
	LitObject object;
	int arity;
	int upvalue_count;
	LitChunk chunk;
	LitString* name;
} LitFunction;

typedef void (*LitNativeFn)(LitVm *vm);

typedef struct {
	LitObject object;
	LitNativeFn function;
} LitNative;

typedef struct {
	LitObject object;
	LitFunction* function;
	LitUpvalue** upvalues;
	int upvalue_count;
} LitClosure;

LitUpvalue* lit_new_upvalue(LitVm* vm, LitValue* slot);
LitClosure* lit_new_closure(LitVm* vm, LitFunction* function);
LitFunction* lit_new_function(LitVm* vm);
LitNative* lit_new_native(LitVm* vm, LitNativeFn function);

LitString* lit_make_string(LitVm* vm, char* chars, int length);
LitString* lit_copy_string(LitVm* vm, const char* chars, int length);

static inline bool lit_is_object_type(LitValue value, LitObjectType type) {
	return IS_OBJECT(value) && AS_OBJECT(value)->type == type;
}

#endif