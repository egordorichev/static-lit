#ifndef LIT_MEMORY_H
#define LIT_MEMORY_H

#include <lit_common.h>
#include <lit_predefines.h>

#include <vm/lit_value.h>

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_ARRAY(vm, previous, type, old_count, count) (type*) reallocate(vm, previous, sizeof(type) * (old_count), sizeof(type) * (count))
#define FREE_ARRAY(vm, type, pointer, old_count) reallocate(vm, pointer, sizeof(type) * (old_count), 0)
#define ALLOCATE(vm, type, count) (type*) reallocate(vm, NULL, 0, sizeof(type) * (count))
#define FREE(vm, type, pointer) reallocate(vm, pointer, sizeof(type), 0)

// Used by both VM and compiler
void* base_reallocate(LitMemManager* manager, void* previous, size_t old_size, size_t new_size);

#define reallocate(x, b, c, d) base_reallocate(_Generic((x), LitCompiler*: (LitMemManager*)(x), LitVm*: (LitMemManager*)(x), LitMemManager*: x), b, c, d)

// VM only stuff
void lit_gray_object(LitVm* vm, LitObject* object);
void lit_gray_value(LitVm* vm, LitValue value);
void lit_collect_garbage(LitVm* vm);
void lit_free_object(LitMemManager* manager, LitObject* object);
void lit_free_objects(LitMemManager* manager);

#endif