#include <string.h>

#include "lit_object.h"
#include "lit_memory.h"
#include "lit.h"
#include "lit_table.h"

#define ALLOCATE_OBJECT(vm, type, object_type) \
    (type*) allocate_object(vm, sizeof(type), object_type)

static LitObject* allocate_object(LitVm* vm, size_t size, LitObjectType type) {
	LitObject* object = (LitObject*) reallocate(vm, NULL, 0, size);
	object->type = type;
	object->dark = false;

	object->next = vm->objects;
	vm->objects = object;

	return object;
}

static LitString* allocate_string(LitVm* vm, char* chars, int length, uint32_t hash) {
	LitString* string = ALLOCATE_OBJECT(vm, LitString, OBJECT_STRING);

	string->length = length;
	string->chars = chars;
	string->hash = hash;

	lit_push(vm, MAKE_OBJECT_VALUE(string));
	lit_table_set(vm, &vm->strings, string, NIL_VALUE);
	lit_pop(vm);

	return string;
}

static uint32_t hash_string(const char* key, int length) {
	// FNV-1a hash. See: http://www.isthe.com/chongo/tech/comp/fnv/
	uint32_t hash = 2166136261u;

	for (int i = 0; i < length; i++) {
		hash ^= key[i];
		hash *= 16777619;
	}

	return hash;
}

LitString* lit_make_string(LitVm* vm, char* chars, int length) {
	uint32_t hash = hash_string(chars, length);
	LitString* interned = lit_table_find(&vm->strings, chars, length, hash);

	if (interned != NULL) {
		return interned;
	}

	return allocate_string(vm, chars, length, hash);
}

LitString* lit_copy_string(LitVm* vm, const char* chars, int length) {
	uint32_t hash = hash_string(chars, length);
	LitString* interned = lit_table_find(&vm->strings, chars, length, hash);

	if (interned != NULL) {
		return interned;
	}

	char* heap_chars = ALLOCATE(vm, char, length + 1);
	memcpy(heap_chars, chars, length);
	heap_chars[length] = '\0';

	return allocate_string(vm, heap_chars, length, hash);
}