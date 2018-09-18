#ifndef LIT_MEMORY_H
#define LIT_MEMORY_H

#include "lit_common.h"
#include "lit_vm.h"

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_ARRAY(vm, previous, type, old_count, count) \
	(type*) reallocate(vm, previous, sizeof(type) * (old_count), sizeof(type) * (count))

#define FREE_ARRAY(vm, type, pointer, old_count) \
   reallocate(vm, pointer, sizeof(type) * (old_count), 0)

void* reallocate(LitVm* vm, void* previous, size_t old_size, size_t new_size);

#endif