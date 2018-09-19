#include <malloc.h>

#include "lit_memory.h"
#include "lit.h"

void* reallocate(LitVm* vm, void* previous, size_t old_size, size_t new_size) {
	vm->bytes_allocated += new_size - old_size;

	if (new_size > old_size) {
		if (vm->bytes_allocated > vm->next_gc) {
			lit_collect_garbage(vm);
		}
	}

	if (new_size == 0) {
		free(previous);
		return NULL;
	}

	return realloc(previous, new_size);
}

void lit_gray_object(LitVm* vm, LitObject* object) {

}

void lit_gray_value(LitVm* vm, LitValue value) {

}

void lit_collect_garbage(LitVm* vm) {

}

void lit_free_objects(LitVm* vm) {

}