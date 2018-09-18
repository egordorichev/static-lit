#ifndef LIT_ARRAY_H
#define LIT_ARRAY_H

#include "lit_vm.h"
#include "lit_value.h"

typedef struct {
	int capacity;
	int count;
	LitValue* values;
} LitArray;

void lit_init_array(LitArray *array);
void lit_free_array(LitVm* vm, LitArray *array);

void lit_array_write(LitVm* vm, LitArray *array, LitValue value);

#endif