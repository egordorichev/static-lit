#ifndef LIT_VALUE_HPP
#define LIT_VALUE_HPP

#include <cstdio>

#include "lit_config.hpp"

enum LitValueType {
	VAL_BOOL,
	VAL_NIL,
	VAL_NUMBER
};

struct LitValue {
	LitValueType type;

	union {
		bool boolean;
		LitNumber number;
	} as;
};

#define MAKE_BOOL_VALUE(value) ((LitValue){ VAL_BOOL, { .boolean = value } })
#define NIL_VAL ((LitValue) { VAL_NIL, { .number = 0 } })
#define MAKE_NUMBER_VALUE(value) ((LitValue){ VAL_NUMBER, { .number = value } })

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)

void lit_print_value(LitValue value);
bool lit_values_are_equal(LitValue a, LitValue b);

#endif