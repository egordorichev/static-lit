#ifndef LIT_CHUNK_H
#define LIT_CHUNK_H

#include "lit_common.h"
#include "lit_vm.h"
#include "lit_array.h"

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
	OP_FALSE = 12
} LitOpCode;

typedef struct {
	int count;
	int capacity;
	int* lines;
	uint8_t* code;

	LitArray constants;
} LitChunk;

void lit_init_chunk(LitChunk* chunk);
void lit_free_chunk(LitVm* vm, LitChunk* chunk);

void lit_chunk_write(LitVm* vm, LitChunk* chunk, uint8_t byte, int line);
int lit_chunk_add_constant(LitVm* vm, LitChunk* chunk, LitValue constant);


#endif