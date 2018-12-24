#include <stdio.h>

#include <util/lit_array.h>
#include <vm/lit_memory.h>
#include <lit.h>

DEFINE_ARRAY(LitArray, LitValue, array)

void lit_gray_array(LitVm* vm, LitArray* array) {
	for (int i = 0; i < array->count; i++) {
		lit_gray_value(vm, array->values[i]);
	}
}