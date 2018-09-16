#include <cstdio>
#include "value.hpp"
#include "chunk.hpp"

LitValueArray::LitValueArray() {
	count = 0;
	capacity = 0;
	values = nullptr;
}

LitValueArray::~LitValueArray() {
	free();
}

void LitValueArray::free() {
	FREE_ARRAY(LitValue, values, capacity);

	count = 0;
	capacity = 0;
	values = nullptr;
}

void LitValueArray::write(LitValue value) {
	if (capacity < count + 1) {
		int oldCapacity = capacity;
		capacity = GROW_CAPACITY(oldCapacity);
		values = GROW_ARRAY(values, LitValue, oldCapacity, capacity);
	}

	values[count] = value;
	count ++;
}

void lit_print_value(LitValue value) {
	printf("%g", value);
}