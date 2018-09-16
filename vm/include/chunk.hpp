#ifndef LIT_CHUNK_HPP
#define LIT_CHUNK_HPP

#include "config.hpp"
#include "value.hpp"

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_ARRAY(previous, type, oldCount, count) (type*)reallocate(previous, sizeof(type) * (oldCount), sizeof(type) * (count))
#define FREE_ARRAY(type, pointer, oldCount) reallocate(pointer, sizeof(type) * (oldCount), 0)

void *reallocate(void *previous, size_t old_size, size_t new_size);

// Operation codes
enum {
	OP_RETURN
} LitOpCode;

typedef struct {
	int count;
	int capacity;
	uint8_t *code;
	LitValueArray constants;
} LitChunk;

void lit_init_chunk(LitChunk *chunk);
void lit_write_chunk(LitChunk *chunk, uint8_t code);
int lit_add_constant(LitChunk *chunk, LitValue value);
void lit_free_chunk(LitChunk *chunk);

#endif