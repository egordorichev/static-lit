#include <cstring>
#include "lit_object.hpp"
#include "lit_vm.hpp"
#include "lit_value.hpp"
#include "lit_gc.hpp"

#define ALLOCATE_OBJECT(vm, type, objectType) (type*) allocate_object(vm, sizeof(type), objectType)

static LitObject* allocate_object(LitVm* vm, size_t size, LitObjectType type) {
  LitObject* object = (LitObject*) reallocate(NULL, 0, size);
  object->type = type;
  object->dark = false;

  object->next = vm->objects;
  vm->objects = object;

#ifdef DEBUG_TRACE_GC
  printf("%p allocate %ld for %d\n", object, size, type);
#endif

  return object;
}

static uint32_t hashString(const char* key, int length) {
  // FNV-1a hash. See: http://www.isthe.com/chongo/tech/comp/fnv/
  uint32_t hash = 2166136261u;

  for (int i = 0; i < length; i++) {
    hash ^= key[i];
    hash *= 16777619;
  }

  return hash;
}

LitString* lit_alloc_string(LitVm* vm, char* chars, int length, uint32_t hash) {
  LitString* string = ALLOCATE_OBJECT(vm, LitString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;

  vm->push(MAKE_OBJECT_VALUE(string));
  vm->strings.set(string, NIL_VALUE);
  vm->pop();

  return string;
}

LitString* lit_take_string(LitVm* vm, const char* chars, int length) {
  uint32_t hash = hashString(chars, length);
  LitString* interned = vm->strings.find(chars, length, hash);

  if (interned != nullptr) {
    return interned;
  }

  return lit_alloc_string(vm, (char*) chars, length, hash);
}

LitString* lit_copy_string(LitVm* vm, const char* chars, int length) {
  uint32_t hash = hashString(chars, length);
  LitString* interned = vm->strings.find(chars, length, hash);

  if (interned != nullptr) {
    return interned;
  }

  char* heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';

  return lit_alloc_string(vm, heapChars, length, hash);
}