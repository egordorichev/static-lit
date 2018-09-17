#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdlib>
#include <cassert>

#define UNREACHABLE() printf("Unreachable!\n"); assert(false);

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_ARRAY(previous, type, oldCount, count) (type*) reallocate((type*) previous, sizeof(type) * (oldCount), sizeof(type) * (count))
#define FREE_ARRAY(type, pointer, oldCount) reallocate(pointer, sizeof(type) * (oldCount), 0)

void* reallocate(void* previous, size_t old_size, size_t new_size);

#endif