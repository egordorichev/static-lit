#ifndef LIT_ARRAY_H
#define LIT_ARRAY_H

#include "lit_vm.h"
#include "lit_value.h"
#include "lit_ast.h"

typedef struct {
	int capacity;
	int count;
	LitValue* values;
} LitArray;

void lit_init_array(LitArray* array);
void lit_free_array(LitVm* vm, LitArray* array);
void lit_array_write(LitVm* vm, LitArray* array, LitValue value);

typedef struct {
		int capacity;
		int count;
		LitStatement** values;
} LitStatements;

void lit_init_statements(LitStatements* array);
void lit_free_statements(LitVm* vm, LitStatements* array);
void lit_statements_write(LitVm* vm, LitStatements* array, LitStatement* statement);

#endif