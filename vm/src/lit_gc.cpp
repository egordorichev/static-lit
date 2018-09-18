#include "lit_gc.hpp"
#include "lit_vm.hpp"
#include "lit_object.hpp"

void* reallocate(void* previous, size_t old_size, size_t new_size) {
  /*vm->bytes_allocated += new_size - old_size;

  if (new_size > old_size) {
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#endif

    if (vm->bytes_allocated > vm->next_gc) {
      collect_garbage();
    }
  }*/

  if (new_size == 0) {
    free(previous);

    return nullptr;
  }

  return realloc(previous, new_size);
}

void gray_object(LitVm* vm, LitObject* object) {
  if (object == nullptr) {
    return;
  }

  if (object->dark) {
    return;
  }

#ifdef DEBUG_TRACE_GC
  printf("%p gray ", object);
  printValue(MAKE_OBJECT_VALUE(object));
  printf("\n");
#endif

  object->dark = true;

  if (vm->gray_capacity < vm->gray_count + 1) {
    vm->gray_capacity = GROW_CAPACITY(vm->gray_capacity);
    vm->gray_stack = static_cast<LitObject**>(realloc(vm->gray_stack, sizeof(LitObject*) * vm->gray_capacity));
  }

  vm->gray_stack[vm->gray_count++] = object;
}

void gray_value(LitVm* vm, LitValue value) {
  if (IS_OBJECT(value)) {
    gray_object(vm, AS_OBJECT(value));
  }
}

void collect_garbage(LitVm* vm) {

}

void free_objects(LitVm* vm) {

}