#ifndef LIT_VALUE_HPP
#define LIT_VALUE_HPP

#include <cstdio>

#include "lit_config.hpp"

typedef struct _LitObject LitObject;
typedef struct _LitString LitString;

enum LitValueType {
	VAL_BOOL,
	VAL_NIL,
	VAL_NUMBER,
	VAL_OBJECT
};

struct LitValue {
	LitValueType type;

	union {
		bool boolean;
		LitNumber number;
		LitObject *object;
	} as;
};

#define MAKE_BOOL_VALUE(value) ((LitValue){ VAL_BOOL, { .boolean = value } })
#define NIL_VAL ((LitValue) { VAL_NIL, { .number = 0 } })
#define MAKE_NUMBER_VALUE(value) ((LitValue){ VAL_NUMBER, { .number = value } })
#define MAKE_OBJECT_VALUE(obj) ((LitValue){ VAL_OBJECT, { .object = (LitObject*) obj } })

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJECT(value) ((value).as.object)

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJECT(value) ((value).type == VAL_OBJECT)

#define OBJECT_TYPE(value) (AS_OBJECT(value)->type)
#define AS_CSTRING(value) (((LitString*) AS_OBJECT(value))->chars)

char *lit_to_string(LitValue value);
void lit_print_value(LitValue value);
bool lit_values_are_equal(LitValue a, LitValue b);

#endif