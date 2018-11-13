#include <stdio.h>

#include <vm/lit_chunk.h>
#include <vm/lit_memory.h>

void lit_init_chunk(LitChunk* chunk) {
	chunk->count = 0;
	chunk->capacity = 0;
	chunk->code = NULL;
	chunk->line_count = 0;
	chunk->line_capacity = 0;
	chunk->lines = NULL;

	lit_init_array(&chunk->constants);
}

void lit_free_chunk(LitMemManager* manager, LitChunk* chunk) {
	FREE_ARRAY(manager, uint8_t , chunk->code, chunk->capacity);
	FREE_ARRAY(manager, uint64_t , chunk->lines, chunk->line_capacity);

	lit_free_array(manager, &chunk->constants);
	lit_init_chunk(chunk);
}

void lit_chunk_write(LitMemManager* manager, LitChunk* chunk, uint8_t byte, uint64_t line) {
	if (chunk->capacity < chunk->count + 1) {
		uint64_t old_capacity = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(old_capacity);
		chunk->code = GROW_ARRAY(manager, chunk->code, uint8_t, old_capacity, chunk->capacity);
	}

	chunk->code[chunk->count] = byte;
	chunk->count++;

	if (chunk->line_capacity == 0 || (line != chunk->lines[chunk->line_count - 1] && chunk->line_capacity < chunk->line_count + 1)) {
		uint64_t old_capacity = chunk->line_capacity;
		chunk->line_capacity = GROW_CAPACITY(old_capacity);
		chunk->lines = GROW_ARRAY(manager, chunk->lines, uint64_t, old_capacity, chunk->line_capacity);
	}

	if (chunk->line_count < 2 || chunk->lines[chunk->line_count - 1] != line) {
		chunk->lines[chunk->line_count] = 1;
		chunk->lines[chunk->line_count + 1] = line;
		chunk->line_count += 2;
	} else {
		chunk->lines[chunk->line_count - 2] = chunk->lines[chunk->line_count - 2] + 1;
	}
}

int lit_chunk_add_constant(LitMemManager* manager, LitChunk* chunk, LitValue constant) {
	lit_array_write(manager, &chunk->constants, constant);
	return chunk->constants.count - 1;
}

uint64_t lit_chunk_get_line(LitChunk* chunk, uint64_t offset) {
	uint64_t i = 0;
	uint64_t total = 0;

	while (i < chunk->line_count) {
		total += chunk->lines[i];

		if (total >= offset) {
			return chunk->lines[i + 1];
		}

		i += 2;
	}

	return 0;
}