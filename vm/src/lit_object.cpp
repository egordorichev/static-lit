#include <cstring>
#include "lit_object.hpp"
#include "lit_vm.hpp"
#include "lit_value.hpp"

#define ALLOCATE_OBJECT(type, objectType) (type*) allocate_object(sizeof(type), objectType)

static LitObject* allocate_object(size_t size, LitObjectType type) {
  LitObject* object = (LitObject*) reallocate(NULL, 0, size);
  object->type = type;
  object->dark = false;

  object->next = lit_get_active_vm()->objects;
  lit_get_active_vm()->objects = object;

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

LitString* lit_alloc_string(char* chars, int length, uint32_t hash) {
  LitString* string = ALLOCATE_OBJECT(LitString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;

  LitVm* vm = lit_get_active_vm();

  vm->push(MAKE_OBJECT_VALUE(string));
  vm->strings.set(string, NIL_VALUE);
  vm->pop();

  return string;
}

LitString* lit_take_string(const char* chars, int length) {
  uint32_t hash = hashString(chars, length);
  LitString* interned = lit_get_active_vm()->strings.find(chars, length, hash);

  if (interned != nullptr) {
    return interned;
  }

  return lit_alloc_string((char*) chars, length, hash);
}

LitString* lit_copy_string(const char* chars, int length) {
  uint32_t hash = hashString(chars, length);
  LitString* interned = lit_get_active_vm()->strings.find(chars, length, hash);

  if (interned != nullptr) {
    return interned;
  }

  char* heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';

  return lit_alloc_string(heapChars, length, hash);
}