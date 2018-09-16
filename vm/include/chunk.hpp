#ifndef LIT_CHUNK_HPP
#define LIT_CHUNK_HPP

#include "config.hpp"
#include "value.hpp"

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_ARRAY(previous, type, oldCount, count) (type *) reallocate((type *) previous, sizeof(type) * (oldCount), sizeof(type) * (count))
#define FREE_ARRAY(type, pointer, oldCount) reallocate(pointer, sizeof(type) * (oldCount), 0)

void *reallocate(void *previous, size_t old_size, size_t new_size);

// Operation codes
enum {
	OP_CONSTANT,
	OP_RETURN
} LitOpCode;

class LitChunk {
	public:
		LitChunk();
		~LitChunk();

		void write(uint8_t cd);
		int add_constant(LitValue value);

		int get_count();
		int get_capacity();
		uint8_t *get_code();
		LitValueArray *get_constants();
	private:
		int count;
		int capacity;
		uint8_t *code;
		LitValueArray constants;
};
#endif