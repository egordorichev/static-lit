#include <cstdlib>

#include "chunk.hpp"

void *reallocate(void *previous, size_t old_size, size_t new_size) {
	if (new_size == 0) {
		free(previous);
		return nullptr;
	}

	return realloc(previous, new_size);
}

LitChunk::LitChunk() {
	count = 0;
	capacity = 0;
	code = nullptr;
	lines = nullptr;
}

LitChunk::~LitChunk() {
	FREE_ARRAY(uint8_t, code, capacity);
	FREE_ARRAY(int, lines, capacity);

	constants.free();
	count = 0;
	capacity = 0;
	code = nullptr;
	lines = nullptr;
}

void LitChunk::write(uint8_t cd, int line) {
	if (capacity < count + 1) {
		int old_capacity = capacity;
		capacity = GROW_CAPACITY(old_capacity);
		code = GROW_ARRAY(cd, uint8_t, old_capacity, capacity);
		lines = GROW_ARRAY(lines, int, old_capacity, capacity);
	}

	code[count] = cd;
	lines[count] = line;
	count ++;
}

int LitChunk::add_constant(LitValue value) {
	constants.write(value);
	return constants.get_count() - 1;
}