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
	OP_GET_LOCAL = 20,
	OP_SET_LOCAL = 21,
	OP_GET_UPVALUE = 22,
	OP_SET_UPVALUE = 23,
	OP_JUMP = 24,
	OP_JUMP_IF_FALSE = 25,
	OP_LOOP = 26,
	OP_CLOSURE = 27,
	OP_SUBCLASS = 28,
	OP_CLASS = 29,
	OP_METHOD = 30,
	OP_GET_PROPERTY = 31,
	OP_SET_PROPERTY = 32,
	OP_INVOKE = 33,
	OP_CALL = 34
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