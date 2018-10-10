#include <stdio.h>
#include "lit_chunk.h"
#include "lit_memory.h"

void lit_init_chunk(LitChunk* chunk) {
	chunk->count = 0;
	chunk->capacity = 0;
	chunk->code = NULL;

	lit_init_array(&chunk->constants);
}

void lit_free_chunk(LitVm* vm, LitChunk* chunk) {
	FREE_ARRAY(vm, uint8_t, chunk->code, chunk->capacity);

	lit_init_chunk(chunk);
	lit_free_array(vm, &chunk->constants);
}

void lit_chunk_write(LitVm* vm, LitChunk* chunk, uint8_t byte) {
	if (chunk->capacity < chunk->count + 1) {
		int oldCapacity = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(oldCapacity);
		chunk->code = GROW_ARRAY(vm, chunk->code, uint8_t, oldCapacity, chunk->capacity);
	}

	chunk->code[chunk->count] = byte;
	chunk->count++;
}

int lit_chunk_add_constant(LitVm* vm, LitChunk* chunk, LitValue constant) {
	lit_array_write(vm, &chunk->constants, constant);
	return chunk->constants.count - 1;
}