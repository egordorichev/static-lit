#ifndef LIT_OBJECT_H
#define LIT_OBJECT_H

#include "lit_common.h"
#include "lit_value.h"
#include "lit_vm.h"

#define OBJECT_TYPE(value) (AS_OBJECT(value)->type)
#define IS_STRING(value) lit_is_object_type(value, OBJECT_STRING)

#define AS_STRING(value) ((LitString*) AS_OBJECT(value))
#define AS_CSTRING(value) (((LitString*) AS_OBJECT(value))->chars)

typedef enum {
	OBJECT_STRING,
	OBJECT_UPVALUE
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

LitString* lit_make_string(LitVm* vm, char* chars, int length);
LitString* lit_copy_string(LitVm* vm, const char* chars, int length);

static inline bool lit_is_object_type(LitValue value, LitObjectType type) {
	return IS_OBJECT(value) && AS_OBJECT(value)->type == type;
}

#endif