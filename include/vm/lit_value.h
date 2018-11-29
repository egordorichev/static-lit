#ifndef LIT_VALUE_H
#define LIT_VALUE_H

#include <stdio.h>

#include <lit_common.h>
#include <lit_predefines.h>

#include <util/lit_array.h>

#define SIGN_BIT ((uint64_t) 1 << 63)
#define QNAN ((uint64_t) 0x7ffc000000000000)

#define TAG_NIL 1
#define TAG_FALSE 2
#define TAG_TRUE 3
#define TAG_CHAR 4

typedef uint64_t LitValue;

#define MASK_TAG (7)
#define GET_TAG(value) ((int) ((value) & MASK_TAG))
#define IS_FALSE(v) (GET_TAG(v) == TAG_FALSE)
#define IS_TRUE(v) (GET_TAG(v) == TAG_TRUE)
#define IS_CHAR(v) (GET_TAG(v) == TAG_CHAR)
#define IS_BOOL(v) (IS_TRUE(v) || IS_FALSE(v))
#define IS_NIL(v) ((v) == NIL_VALUE)
#define IS_NUMBER(v) (((v) & QNAN) != QNAN)
#define IS_OBJECT(v) (((v) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(v) ((v) == TRUE_VALUE)
#define AS_NUMBER(v) lit_value_to_num(v)
#define AS_OBJECT(v) ((LitObject*)(uintptr_t)((v) & ~(SIGN_BIT | QNAN)))

#define MAKE_BOOL_VALUE(boolean) ((boolean) ? TRUE_VALUE : FALSE_VALUE)
#define FALSE_VALUE ((LitValue) (uint64_t) (QNAN | TAG_FALSE))
#define TRUE_VALUE ((LitValue) (uint64_t) (QNAN | TAG_TRUE))
#define NIL_VALUE ((LitValue) (uint64_t) (QNAN | TAG_NIL))
#define MAKE_NUMBER_VALUE(num) lit_num_to_value(num)
#define MAKE_CHAR_VALUE(num) lit_char_to_value(num)

#define MAKE_OBJECT_VALUE(obj) (LitValue) (SIGN_BIT | QNAN | (uint64_t) (uintptr_t) (obj))

typedef union {
	uint64_t bits64;
	uint32_t bits32[2];
	uint16_t bits16[4];
	uint8_t bits8[8];
	double num;
} DoubleUnion;

static inline LitValue lit_char_to_value(unsigned char ch) {
	DoubleUnion data;
	data.bits64 = QNAN | 5;
	data.bits16[0] = (uint16_t) ch;

	printf("Made value with tag %i\n", GET_TAG(data.bits64));

	return data.bits64;
}

static inline double lit_value_to_num(LitValue value) {
	DoubleUnion data;
	data.bits64 = value;
	return data.num;
}

static inline LitValue lit_num_to_value(double num) {
	DoubleUnion data;
	data.num = num;
	return data.bits64;
}

bool lit_is_false(LitValue value);
char *lit_to_string(LitVm* vm, LitValue value);

#endif