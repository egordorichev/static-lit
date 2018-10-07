#include <malloc.h>

#include "lit_memory.h"
#include "lit.h"
#include "lit_debug.h"
#include "lit_object.h"

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(LitVm* vm, void* previous, size_t old_size, size_t new_size) {
	vm->bytes_allocated += new_size - old_size;

	if (new_size > old_size && vm->bytes_allocated > vm->next_gc) {
		lit_collect_garbage(vm);
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
	printf("%p gray %s\n", object, lit_to_string(vm, MAKE_OBJECT_VALUE(object)));
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
	printf("%p blacken %s\n", object, lit_to_string(vm, MAKE_OBJECT_VALUE(object)));
#endif

	switch (object->type) {
		case OBJECT_FUNCTION: {
			LitFunction* function = (LitFunction*) object;
			lit_gray_object(vm, (LitObject*) function->name);
			gray_array(vm, &function->chunk.constants);
			break;
		}
		case OBJECT_CLOSURE: {
			LitClosure* closure = (LitClosure*) object;
			lit_gray_object(vm, (LitObject*) closure->function);

			for (int i = 0; i < closure->upvalue_count; i++) {
				lit_gray_object(vm, (LitObject*) closure->upvalues[i]);
			}

			break;
		}
		case OBJECT_UPVALUE: lit_gray_value(vm, ((LitUpvalue*) object)->closed); break;
		case OBJECT_NATIVE: case OBJECT_STRING: break;
		case OBJECT_CLASS: {
			LitClass* class = (LitClass*) object;
			lit_gray_object(vm, (LitObject*) class->name);
			lit_gray_object(vm, (LitObject*) class->super);
			lit_table_gray(vm, &class->methods);
			break;
		}
		case OBJECT_INSTANCE: {
			LitInstance* instance = (LitInstance*) object;
			lit_gray_object(vm, (LitObject*) instance->type);
			lit_table_gray(vm, &instance->fields);
			break;
		}
		case OBJECT_BOUND_METHOD: {
			LitMethod* bound = (LitMethod*) object;
			lit_gray_value(vm, bound->receiver);
			lit_gray_object(vm, (LitObject*) bound->method);
			break;
		}
		default: UNREACHABLE();
	}
}

static void free_object(LitVm* vm, LitObject* object) {
#ifdef DEBUG_TRACE_GC
	printf("%p free %s\n", object, lit_to_string(vm, MAKE_OBJECT_VALUE(object)));
#endif

	switch (object->type) {
		case OBJECT_STRING: {
			LitString* string = (LitString*) object;
			FREE_ARRAY(vm, char, string->chars, string->length + 1);
			FREE(vm, LitObject, object);
			break;
		}
		case OBJECT_CLOSURE: {
			LitClosure* closure = (LitClosure*) object;
			FREE_ARRAY(vm, LitValue, closure->upvalues, closure->upvalue_count);
			FREE(vm, LitClosure, object);
			break;
		}
		case OBJECT_FUNCTION: {
			LitFunction* function = (LitFunction*) object;
			lit_free_chunk(vm, &function->chunk);
			FREE(vm, LitFunction, object);
			break;
		}
		case OBJECT_NATIVE: FREE(vm, LitNative, object); break;
		case OBJECT_UPVALUE: FREE(vm, LitUpvalue, object); break;
		case OBJECT_BOUND_METHOD: FREE(vm, LitMethod, object); break;
		case OBJECT_CLASS: {
			lit_free_table(vm, &((LitClass*) object)->methods);
			FREE(vm, LitClass, object);
			break;
		}
		case OBJECT_INSTANCE: {
			lit_free_table(vm, &((LitInstance*) object)->fields);
			FREE(vm, LitInstance, object);
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

	for (int i = 0; i < vm->frame_count; i++) {
		lit_gray_object(vm, (LitObject*) vm->frames[i].closure);
	}

	for (LitUpvalue* upvalue = vm->open_upvalues; upvalue != NULL; upvalue = upvalue->next) {
		lit_gray_object(vm, (LitObject*) upvalue);
	}

	lit_table_gray(vm, &vm->globals);

	lit_gray_object(vm, (LitObject*) vm->init_string);

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