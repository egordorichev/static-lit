#include <stdio.h>

#include "lit_array.h"
#include "lit_memory.h"

void lit_init_array(LitArray* array) {
	array->values = NULL;
	array->capacity = 0;
	array->count = 0;
}

void lit_free_array(LitVm* vm, LitArray* array) {
	FREE_ARRAY(vm, LitValue, array->values, array->capacity);
	lit_init_array(array);
}

void lit_array_write(LitVm* vm, LitArray* array, LitValue value) {
	if (array->capacity < array->count + 1) {
		int old_capacity = array->capacity;
		array->capacity = GROW_CAPACITY(old_capacity);
		array->values = GROW_ARRAY(vm, array->values, LitValue, old_capacity, array->capacity);
	}

	array->values[array->count] = value;
	array->count++;
}

void lit_init_statements(LitStatements* array) {
	array->values = NULL;
	array->capacity = 0;
	array->count = 0;
}

void lit_free_statements(LitVm* vm, LitStatements* array) {
	FREE_ARRAY(vm, LitStatement*, array->values, array->capacity);
	lit_init_statements(array);
}

void lit_statements_write(LitVm* vm, LitStatements* array, LitStatement* statement) {
	if (array->capacity < array->count + 1) {
		int old_capacity = array->capacity;
		array->capacity = GROW_CAPACITY(old_capacity);
		array->values = GROW_ARRAY(vm, array->values, LitStatement*, old_capacity, array->capacity);
	}

	array->values[array->count] = statement;
	array->count++;
}
void lit_init_expressions(LitExpressions* array) {
	array->values = NULL;
	array->capacity = 0;
	array->count = 0;
}

void lit_free_expressions(LitVm* vm, LitExpressions* array) {
	FREE_ARRAY(vm, LitExpression*, array->values, array->capacity);
	lit_init_expressions(array);
}

void lit_expressions_write(LitVm* vm, LitExpressions* array, LitExpression* statement) {
	if (array->capacity < array->count + 1) {
		int old_capacity = array->capacity;
		array->capacity = GROW_CAPACITY(old_capacity);
		array->values = GROW_ARRAY(vm, array->values, LitExpression*, old_capacity, array->capacity);
	}

	array->values[array->count] = statement;
	array->count++;
}
