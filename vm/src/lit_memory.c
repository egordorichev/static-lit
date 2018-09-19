#include <malloc.h>

#include "lit_memory.h"
#include "lit.h"
#include "lit_debug.h"
#include "lit_object.h"

#define GC_HEAP_GROW_FACTOR 2

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
	if (object == NULL) {
		return;
	}

	if (object->dark) {
		return;
	}

#ifdef DEBUG_TRACE_GC
	printf("%p gray %s\n", object, lit_to_string(MAKE_OBJECT_VALUE(object)));
#endif

	object->dark = true;

	if (vm->gray_capacity < vm->gray_count + 1) {
		vm->gray_capacity = GROW_CAPACITY(vm->gray_capacity);
		vm->gray_stack = realloc(vm->gray_stack, sizeof(LitObject*) * vm->gray_capacity);
	}

	vm->gray_stack[vm->gray_count++] = object;
}

void lit_gray_value(LitVm* vm, LitValue value) {
	if (IS_OBJECT(value)) {
		lit_gray_object(vm, AS_OBJECT(value));
	}
}

static void gray_array(LitVm* vm, LitArray* array) {
	for (int i = 0; i < array->count; i++) {
		lit_gray_value(vm, array->values[i]);
	}
}

static void blacken_object(LitVm* vm, LitObject* object) {
#ifdef DEBUG_TRACE_GC
	printf("%p blacken %s\n", object, lit_to_string(MAKE_OBJECT_VALUE(object)));
#endif

	switch (object->type) {

	}
}

static void free_object(LitVm* vm, LitObject* object) {
#ifdef DEBUG_TRACE_GC
	printf("%p free %s\n", object, lit_to_string(MAKE_OBJECT_VALUE(object)));
#endif

	switch (object->type) {
		case OBJECT_STRING: {
			LitString* string = (LitString*) object;
			FREE_ARRAY(vm, char, string->chars, string->length + 1);
			FREE(vm, LitObject, object);
			break;
		}
		default: UNREACHABLE();
	}
}

void lit_collect_garbage(LitVm* vm) {
#ifdef DEBUG_TRACE_GC
	printf("-- gc begin\n");
	size_t before = vm->bytes_allocated;
#endif

	for (LitValue* slot = vm->stack; slot < vm->stack_top; slot++) {
		lit_gray_value(vm, *slot);
	}

	/*for (int i = 0; i < vm.frameCount; i++) {
		grayObject((Obj*)vm.frames[i].closure);
	}*/

	/*for (ObjUpvalue* upvalue = vm.openUpvalues;
	     upvalue != NULL;
	     upvalue = upvalue->next) {
		grayObject((Obj*)upvalue);
	}*/

	//grayTable(&vm.globals);
	//grayCompilerRoots();

	// grayObject((Obj*)vm.initString);

	while (vm->gray_count > 0) {
		LitObject* object = vm->gray_stack[--vm->gray_count];
		blacken_object(vm, object);
	}

	lit_table_remove_white(vm, &vm->strings);
	LitObject** object = &vm->objects;

	while (*object != NULL) {
		if (!((*object)->dark)) {
			LitObject* unreached = *object;
			*object = unreached->next;
			free_object(vm, unreached);
		} else {
			(*object)->dark = false;
			object = &(*object)->next;
		}
	}

	vm->next_gc = vm->bytes_allocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_TRACE_GC
	printf("-- gc collected %ld bytes (from %ld to %ld) next at %ld\n",
		before - vm->bytes_allocated, before, vm->bytes_allocated, vm->next_gc);
#endif
}

void lit_free_objects(LitVm* vm) {
	LitObject* object = vm->objects;

	while (object != NULL) {
		LitObject* next = object->next;
		free_object(vm, object);
		object = next;
	}

	free(vm->gray_stack);
}