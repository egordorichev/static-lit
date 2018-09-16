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
}

LitChunk::~LitChunk() {
	FREE_ARRAY(uint8_t, code, capacity);

	constants.free();
	count = 0;
	capacity = 0;
	code = nullptr;
}

void LitChunk::write(uint8_t cd) {
	if (capacity < count + 1) {
		int old_cpacity = capacity;
		capacity = GROW_CAPACITY(old_cpacity);
		code = GROW_ARRAY(cd, uint8_t, old_cpacity, capacity);
	}

	code[count] = cd;
	count ++;
}

int LitChunk::add_constant(LitValue value) {
	constants.write(value);
	return constants.get_count() - 1;
}

int LitChunk::get_count() {
	return count;
}

int LitChunk::get_capacity() {
	return capacity;
}

uint8_t *LitChunk::get_code() {
	return code;
}

LitValueArray *LitChunk::get_constants() {
	return &constants;
}