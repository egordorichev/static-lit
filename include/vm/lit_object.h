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
#define IS_NATIVE_METHOD(value) lit_is_object_type(value, OBJECT_NATIVE_METHOD)
#define IS_CLASS(value) lit_is_object_type(value, OBJECT_CLASS)
#define IS_INSTANCE(value) lit_is_object_type(value, OBJECT_INSTANCE)
#define IS_FIBER(value) lit_is_object_type(value, OBJECT_FIBER)

#define AS_CLOSURE(value) ((LitClosure*) AS_OBJECT(value))
#define AS_FUNCTION(value) ((LitFunction*) AS_OBJECT(value))
#define AS_NATIVE(value) (((LitNative*) AS_OBJECT(value))->function)
#define AS_NATIVE_METHOD(value) (((LitNativeMethod*) AS_OBJECT(value))->method)
#define AS_METHOD(value) ((LitMethod*) AS_OBJECT(value))
#define AS_CLASS(value) ((LitClass*) AS_OBJECT(value))
#define AS_INSTANCE(value) ((LitInstance*) AS_OBJECT(value))
#define AS_STRING(value) ((LitString*) AS_OBJECT(value))
#define AS_CSTRING(value) (((LitString*) AS_OBJECT(value))->chars)
#define AS_UPVALUE(value) ((LitUpvalue*) AS_OBJECT(value))
#define AS_FIBER(value) ((LitFiber*) AS_OBJECT(value))

typedef enum {
	OBJECT_STRING,
	OBJECT_UPVALUE,
	OBJECT_FUNCTION,
	OBJECT_NATIVE,
	OBJECT_CLOSURE,
	OBJECT_BOUND_METHOD,
	OBJECT_CLASS,
	OBJECT_INSTANCE,
	OBJECT_NATIVE_METHOD,
	OBJECT_FIBER
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

LitString* lit_new_string(LitMemManager* manager, int length);
void lit_hash_string(LitString* string);
LitString* lit_make_string(LitMemManager* manager, char* chars, int length);
LitString* lit_copy_string(LitMemManager* manager, const char* chars, size_t length);
LitString* lit_format_string(LitMemManager* manager, const char* format, ...);
char* lit_format_cstring(LitMemManager* manager, const char* format, ...);

typedef struct sLitUpvalue {
	LitObject object;

	LitValue* value;
	LitValue closed;
	struct sLitUpvalue* next;
} LitUpvalue;

LitUpvalue* lit_new_upvalue(LitMemManager* manager, LitValue* slot);

typedef struct {
	LitObject object;

	int arity;
	int upvalue_count;

	LitChunk chunk;
	LitValue registers[256];
	LitString* name;
} LitFunction;

LitFunction* lit_new_function(LitMemManager* manager);

typedef int (*LitNativeFn)(LitVm *vm, LitValue* args, int count);

typedef struct {
	LitNativeFn function;
	const char* name;
	const char* signature;
} LitNativeRegistry;

typedef struct {
	LitObject object;
	LitNativeFn function;
} LitNative;

LitNative* lit_new_native(LitMemManager* manager, LitNativeFn function);

typedef struct {
	LitObject object;

	LitFunction* function;
	LitUpvalue** upvalues;
	int upvalue_count;
} LitClosure;

LitClosure* lit_new_closure(LitMemManager* manager, LitFunction* function);

typedef struct {
	LitClosure* closure;
	uint8_t* ip;
	LitValue* slots;
} LitFrame;

typedef struct sLitFiber {
	LitObject object;

	LitFrame* frames;
	struct sLitFiber* caller;

	int frame_capacity;
	int frame_count;
	bool abort;
} LitFiber;

LitFiber* lit_new_fiber(LitMemManager* manager, LitClosure* closure);
void lit_append_frame(LitMemManager* manager, LitFiber* fiber, LitClosure* closure);

typedef struct sLitClass {
	LitObject object;

	LitString* name;
	struct sLitClass* super;
	LitTable methods;
	LitTable static_methods;
	LitTable fields;
	LitTable static_fields;
} LitClass;

LitClass* lit_new_class(LitMemManager* manager, LitString* name, LitClass* super);

typedef struct {
	LitObject object;

	LitClass* type;
	LitTable fields;
} LitInstance;

LitInstance* lit_new_instance(LitMemManager* manager, LitClass* class);

typedef struct {
	LitObject object;

	LitValue receiver;
	LitClosure* method;
} LitMethod;

LitMethod* lit_new_bound_method(LitMemManager* manager, LitValue receiver, LitClosure* method);

typedef int (*LitNativeMethodFn)(LitVm *vm, LitValue instance, const LitValue* args, int count);

typedef struct {
	LitObject object;
	LitNativeMethodFn method;
} LitNativeMethod;

LitNativeMethod* lit_new_native_method(LitMemManager* manager, LitNativeMethodFn method);

static inline bool lit_is_object_type(LitValue value, LitObjectType type) {
	return IS_OBJECT(value) && AS_OBJECT(value)->type == type;
}

#endif