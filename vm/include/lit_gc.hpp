#ifndef LIT_GC_HPP
#define LIT_GC_HPP

#include <cstdlib>
#include "lit_value.hpp"

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_ARRAY(previous, type, oldCount, count) (type*) reallocate((type*) previous, sizeof(type) * (oldCount), sizeof(type) * (count))
#define FREE_ARRAY(type, pointer, oldCount) reallocate(pointer, sizeof(type) * (oldCount), 0)

#define ALLOCATE(type, count) (type*)reallocate(NULL, 0, sizeof(type) * (count))
#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

void* reallocate(void* previous, size_t old_size, size_t new_size);

void gray_object(LitObject* object);

void gray_value(LitValue value);

void collect_garbage();

void free_objects();

#endif