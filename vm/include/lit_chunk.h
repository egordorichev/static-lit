#ifndef LIT_CHUNK_H
#define LIT_CHUNK_H

#include "lit_common.h"
#include "lit_vm.h"
#include "lit_array.h"

DECLARE_ARRAY(LitArray, LitValue, array)

typedef enum {
	OP_RETURN = 0,
	OP_CONSTANT = 1,
	OP_PRINT = 2,
	OP_NEGATE = 3,
	OP_ADD = 4,
	OP_SUBTRACT = 5,
	OP_MULTIPLY = 6,
	OP_DIVIDE = 7,
	OP_POP = 8,
	OP_NOT = 9,
	OP_NIL = 10,
	OP_TRUE = 11,
	OP_FALSE = 12,
	OP_EQUAL = 13,
	OP_GREATER = 14,
	OP_LESS = 15,
	// We could make those a combination of op codes
	// Yet it will be a bit slower
	OP_GREATER_EQUAL = 16,
	OP_LESS_EQUAL = 17,
	OP_NOT_EQUAL = 18,
	OP_CLOSE_UPVALUE = 19,
	OP_DEFINE_GLOBAL = 20,
	OP_GET_GLOBAL = 21,
	OP_SET_GLOBAL = 22,
	OP_GET_LOCAL = 23,
	OP_SET_LOCAL = 24,
	OP_GET_UPVALUE = 25,
	OP_SET_UPVALUE = 26,
	OP_JUMP = 27,
	OP_JUMP_IF_FALSE = 28,
	OP_LOOP = 29,
	OP_CLOSURE = 30,
	OP_SUBCLASS = 31,
	OP_CLASS = 32,
	OP_METHOD = 33,
	OP_GET_PROPERTY = 34,
	OP_SET_PROPERTY = 35,
	OP_INVOKE = 36,
	OP_CALL = 37,
	OP_DEFINE_PROPERTY = 38,
	OP_DEFINE_METHOD = 39
} LitOpCode;

typedef struct {
	int count;
	int capacity;
	uint8_t* code;

	LitArray constants;
} LitChunk;

void lit_init_chunk(LitChunk* chunk);
void lit_free_chunk(LitVm* vm, LitChunk* chunk);

void lit_chunk_write(LitVm* vm, LitChunk* chunk, uint8_t byte);
int lit_chunk_add_constant(LitVm* vm, LitChunk* chunk, LitValue constant);

#endif