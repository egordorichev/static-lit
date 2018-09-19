#ifndef LIT_CHUNK_H
#define LIT_CHUNK_H

#include "lit_common.h"
#include "lit_vm.h"
#include "lit_array.h"

typedef enum {
	OP_RETURN,
	OP_CONSTANT,
	OP_PRINT,
	OP_NEGATE,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_POP
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