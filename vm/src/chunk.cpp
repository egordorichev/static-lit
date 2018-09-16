#include <cstdlib>

#include "chunk.hpp"

void *reallocate(void *previous, size_t old_size, size_t new_size) {
	if (new_size == 0) {
		free(previous);
		return nullptr;
	}

	return realloc(previous, new_size);
}

void lit_init_chunk(LitChunk *chunk) {
	chunk->count = 0;
	chunk->capacity = 0;
	chunk->code = nullptr;
}

void lit_write_chunk(LitChunk *chunk, uint8_t code) {
	if (chunk->capacity < chunk->count + 1) {
		int oldCapacity = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(oldCapacity);
		chunk->code = GROW_ARRAY(chunk->code, uint8_t, oldCapacity, chunk->capacity);
	}

	chunk->code[chunk->count] = code;
	chunk->count ++;
}

void lit_free_chunk(LitChunk *chunk) {
	FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
	lit_init_chunk(chunk);
}