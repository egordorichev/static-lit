#ifndef LIT_VALUE_HPP
#define LIT_VALUE_HPP

#include "config.hpp"

#define LitValue LitNumber

typedef struct {
	int count;
	int capacity;
	LitValue *values;
} LitValueArray;

void lit_init_array(LitValueArray *array);
void lit_write_array(LitValueArray *array, LitValue value);
void lit_free_array(LitValueArray *array);

#endif