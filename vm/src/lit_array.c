#include <stdio.h>

#include "lit_array.h"
#include "lit_memory.h"

void lit_init_array(LitArray *array) {
	array->values = NULL;
	array->capacity = 0;
	array->count = 0;
}

void lit_free_array(LitVm* vm, LitArray *array) {
	FREE_ARRAY(vm, LitValue, array->values, array->capacity);
	lit_init_array(array);
}

void lit_array_write(LitVm* vm, LitArray *array, LitValue value) {
	if (array->capacity < array->count + 1) {
		int old_capacity = array->capacity;
		array->capacity = GROW_CAPACITY(old_capacity);
		array->values = GROW_ARRAY(vm, array->values, LitValue, old_capacity, array->capacity);
	}

	array->values[array->count] = value;
	array->count++;
}