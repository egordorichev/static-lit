#include "value.hpp"
#include "chunk.hpp"

void lit_init_array(LitValueArray *array) {
	array->count = 0;
	array->capacity = 0;
	array->values = nullptr;
}

void lit_write_array(LitValueArray *array, LitValue value) {
	if (array->capacity < array->count + 1) {
		int oldCapacity = array->capacity;
		array->capacity = GROW_CAPACITY(oldCapacity);
		array->values = GROW_ARRAY(array->values, LitValue, oldCapacity, array->capacity);
	}

	array->values[array->count] = value;
	array->count ++;
}

void lit_free_array(LitValueArray *array) {
	
}