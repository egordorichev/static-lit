#ifndef LIT_OBJECT_HPP
#define LIT_OBJECT_HPP

#include "lit_common.hpp"
#include "lit_value.hpp"

#include <cstdint>

typedef enum {
	OBJ_BOUND_METHOD,
	OBJ_CLASS,
	OBJ_CLOSURE,
	OBJ_FUNCTION,
	OBJ_INSTANCE,
	OBJ_NATIVE,
	OBJ_STRING,
	OBJ_UPVALUE
} LitObjectType;

struct _LitObject {
	LitObjectType type;
	bool dark;
	struct _LitObject* next;
};

struct _LitString {
	LitObject object;
	int length;
	char* chars;
	uint32_t hash;
};

LitString* lit_alloc_string(char* chars, int length, uint32_t hash);
LitString* lit_copy_string(const char* chars, int length);
LitString* lit_take_string(const char* chars, int length);

static inline bool lit_is_object_type(LitValue value, LitObjectType type) {
	return IS_OBJECT(value) && AS_OBJECT(value)->type == type;
}

#endif