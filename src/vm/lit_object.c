#include <string.h>
#include <zconf.h>

#include <lit.h>

#include <vm/lit_object.h>
#include <vm/lit_memory.h>
#include <util/lit_table.h>

#define ALLOCATE_OBJECT(manager, type, object_type) \
    (type*) allocate_object(manager, sizeof(type), object_type)

static LitObject* allocate_object(LitMemManager* manager, size_t size, LitObjectType type) {
	LitObject* object = (LitObject*) reallocate(manager, NULL, 0, size);

	object->type = type;
	object->dark = false;
	object->next = manager->objects;

	manager->objects = object;

	return object;
}

LitUpvalue* lit_new_upvalue(LitMemManager* manager, LitValue* slot) {
	LitUpvalue* upvalue = ALLOCATE_OBJECT(manager, LitUpvalue, OBJECT_UPVALUE);

	upvalue->closed = NIL_VALUE;
	upvalue->value = slot;
	upvalue->next = NULL;

	return upvalue;
}

LitClosure* lit_new_closure(LitMemManager* manager, LitFunction* function) {
	LitUpvalue** upvalues = ALLOCATE(manager, LitUpvalue*, function->upvalue_count);

	for (int i = 0; i < function->upvalue_count; i++) {
		upvalues[i] = NULL;
	}

	LitClosure* closure = ALLOCATE_OBJECT(manager, LitClosure, OBJECT_CLOSURE);

	closure->function = function;
	closure->upvalues = upvalues;
	closure->upvalue_count = function->upvalue_count;

	return closure;
}

LitFunction* lit_new_function(LitMemManager* manager) {
	LitFunction* function = ALLOCATE_OBJECT(manager, LitFunction, OBJECT_FUNCTION);

	function->arity = 0;
	function->upvalue_count = 0;
	function->name = NULL;

	lit_init_chunk(&function->chunk);

	return function;
}

LitNative* lit_new_native(LitMemManager* manager, LitNativeFn function) {
	LitNative* native = ALLOCATE_OBJECT(manager, LitNative, OBJECT_NATIVE);
	native->function = function;

	return native;
}

LitMethod* lit_new_bound_method(LitMemManager* manager, LitValue receiver, LitClosure* method) {
	LitMethod* bound = ALLOCATE_OBJECT(manager, LitMethod, OBJECT_BOUND_METHOD);

	bound->receiver = receiver;
	bound->method = method;

	return bound;
}

LitClass* lit_new_class(LitMemManager* manager, LitString* name, LitClass* super) {
	LitClass* class = ALLOCATE_OBJECT(manager, LitClass, OBJECT_CLASS);

	class->name = name;
	class->super = super;

	lit_init_table(&class->methods);
	lit_init_table(&class->static_methods);
	lit_init_table(&class->fields);
	lit_init_table(&class->static_fields);

	return class;
}

LitNativeMethod* lit_new_native_method(LitMemManager* manager, LitNativeMethodFn method) {
	LitNativeMethod* m = ALLOCATE_OBJECT(manager, LitNativeMethod, OBJECT_NATIVE_METHOD);

	m->method = method;

	return m;
}

LitInstance* lit_new_instance(LitMemManager* manager, LitClass* class) {
	LitInstance* instance = ALLOCATE_OBJECT(manager, LitInstance, OBJECT_INSTANCE);

	instance->type = class;

	lit_init_table(&instance->fields);
	lit_table_add_all(manager, &instance->fields, &class->fields);

	return instance;
}

static LitString* allocate_string(LitMemManager* manager, char* chars, int length, uint32_t hash) {
	LitString* string = ALLOCATE_OBJECT(manager, LitString, OBJECT_STRING);

	string->length = length;
	string->chars = chars;
	string->hash = hash;

	lit_table_set(manager, &manager->strings, string, NIL_VALUE);

	return string;
}

static uint32_t hash_string(const char* key, int length) {
	// FNV-1a hash http://www.isthe.com/chongo/tech/comp/fnv/
	uint32_t hash = 2166136261u;

	for (int i = 0; i < length; i++) {
		hash ^= key[i];
		hash *= 16777619;
	}

	return hash;
}

LitString* lit_make_string(LitMemManager* manager, char* chars, int length) {
	uint32_t hash = hash_string(chars, length);
	LitString* interned = lit_table_find(&manager->strings, chars, length, hash);

	if (interned != NULL) {
		return interned;
	}

	return allocate_string(manager, chars, length, hash);
}

LitString* lit_copy_string(LitMemManager* manager, const char* chars, size_t length) {
	uint32_t hash = hash_string(chars, (int) length);
	LitString* interned = lit_table_find(&manager->strings, chars, (int) length, hash);

	if (interned != NULL) {
		return interned;
	}

	char* heap_chars = ALLOCATE(manager, char, length + 1);

	memcpy(heap_chars, chars, length);
	heap_chars[length] = '\0';

	return allocate_string(manager, heap_chars, (int) length, hash);
}

static int get_string_length(const char* format, va_list arg_list) {
	int total_length = 0;

	for (const char* c = format; *c != '\0'; c++) {
		switch (*c) {
			case '$': {
				total_length += strlen(va_arg(arg_list, const char*));
				break;
			}
			case '@': {
				total_length += AS_STRING(va_arg(arg_list, LitValue))->length;
				break;
			}
			case '%': {
				total_length += va_arg(arg_list, LitString*)->length;
				break;
			}
			default: {
				total_length++;
				break;
			}
		}
	}

	return total_length;
}

static char* concat_string(LitMemManager* manager, const char* format, int total_length, va_list arg_list) {
	char* heap_chars = ALLOCATE(manager, char, total_length + 1);
	char* start = heap_chars;

	heap_chars[total_length] = '\0';

	for (const char* c = format; *c != '\0'; c++) {
		switch (*c) {
			case '$': {
				const char* string = va_arg(arg_list, const char*);
				size_t length = strlen(string);
				memcpy(start, string, length);
				start += length;

				break;
			}

			case '@': {
				LitString* string = AS_STRING(va_arg(arg_list, LitValue));
				memcpy(start, string->chars, (size_t) string->length);
				start += string->length;

				break;
			}

			case '%': {
				LitString* string = va_arg(arg_list, LitString*);
				memcpy(start, string->chars, (size_t) string->length);
				start += string->length;

				break;
			}

			default: {
				*start++ = *c;
				break;
			}
		}
	}

	return heap_chars;
}

char* lit_format_cstring(LitMemManager* manager, const char* format, ...) {
	va_list arg_list;

	va_start(arg_list, format);
	size_t total_length = (size_t) get_string_length(format, arg_list);
	va_end(arg_list);

	va_start(arg_list, format);
	char* result = concat_string(manager, format, (int) total_length, arg_list);
	va_end(arg_list);

	return result;
}

LitString* lit_format_string(LitMemManager* manager, const char* format, ...) {
	va_list arg_list;

	va_start(arg_list, format);
	int total_length = get_string_length(format, arg_list);
	va_end(arg_list);

	va_start(arg_list, format);
	char* result = concat_string(manager, format, total_length, arg_list);
	va_end(arg_list);

	return allocate_string(manager, result, total_length, hash_string(result, total_length));
}