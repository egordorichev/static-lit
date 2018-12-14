#include <lit_bindings.h>
#include <std/lit_std.h>

#include <time.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

/*
 * Class metaclass
 */
METHOD(class_getSuper) {
	LitClass* class = AS_CLASS(instance);

	if (class->super == NULL) {
		if (class == vm->object_class || class == vm->class_class) {
			RETURN_NIL
		}

		RETURN_OBJECT(vm->object_class)
	} else {
		RETURN_OBJECT(class->super)
	}
}

START_METHODS(class)
	ADD("getSuper", "Function<Class>", class_getSuper, false)
END_METHODS

/*
 * Object class
 */
METHOD(object_getClass) {
	RETURN_OBJECT(AS_INSTANCE(instance)->type)
}

START_METHODS(object)
	ADD("getClass", "Function<Class>", object_getClass, false)
END_METHODS

/*
 * Bool class
 */
START_METHODS(bool)
END_METHODS

/*
 * Int class
 */

METHOD(int_log) {
	RETURN_NUMBER(log(AS_NUMBER(instance)))
}

START_METHODS(int)
	ADD("log", "Function<double>", int_log, false)
END_METHODS

/*
 * Double class
 */
START_METHODS(double)
END_METHODS

/*
 * Char class
 */
START_METHODS(char)
END_METHODS

/*
 * String class
 */
METHOD(string_toLowerCase) {
	LitString* old = AS_STRING(instance);
	LitString* string = lit_new_string(vm, old->length);

	for (int i = 0; i < old->length; i++) {
		string->chars[i] = (char) tolower(old->chars[i]);
	}

	lit_hash_string(string);
	RETURN_STRING(string)
}

METHOD(string_toUpperCase) {
	LitString* old = AS_STRING(instance);
	LitString* string = lit_new_string(vm, old->length);

	for (int i = 0; i < old->length; i++) {
		string->chars[i] = (char) toupper(old->chars[i]);
	}

	lit_hash_string(string);
	RETURN_STRING(string)
}

METHOD(string_contains) {
	LitString* self = AS_STRING(instance);
	LitString* sub = AS_STRING(args[0]);

	if (self == sub) { // Same string
		RETURN_BOOL(true)
	}

	RETURN_BOOL(strstr(self->chars, sub->chars) != NULL)
}

METHOD(string_endsWith) {
	LitString* self = AS_STRING(instance);
	LitString* sub = AS_STRING(args[0]);

	if (self == sub) { // Same string
		RETURN_BOOL(true)
	}

	if (self->length < sub->length) {
		RETURN_BOOL(false)
	}

	int start = self->length - sub->length;

	for (int i = 0; i < sub->length; i++) {
		if (self->chars[i + start] != sub->chars[i]) {
			RETURN_BOOL(false)
		}
	}

	RETURN_BOOL(true)
}

METHOD(string_startsWith) {
	LitString* self = AS_STRING(instance);
	LitString* sub = AS_STRING(args[0]);

	if (self == sub) { // Same string
		RETURN_BOOL(true)
	}

	if (self->length < sub->length) {
		RETURN_BOOL(false)
	}

	for (int i = 0; i < sub->length; i++) {
		if (self->chars[i] != sub->chars[i]) {
			RETURN_BOOL(false)
		}
	}

	RETURN_BOOL(true)
}

METHOD(string_getLength) {
	RETURN_NUMBER(AS_STRING(instance)->length);
}

METHOD(string_getHash) {
	RETURN_NUMBER(AS_STRING(instance)->hash);
}

START_METHODS(string)
	ADD("toLowerCase", "Function<void>", string_toLowerCase, false)
	ADD("toUpperCase", "Function<void>", string_toUpperCase, false)
	ADD("contains", "Function<String, void>", string_contains, false)
	ADD("startsWith", "Function<String, void>", string_startsWith, false)
	ADD("endsWith", "Function<String, void>", string_endsWith, false)
	ADD("getLength", "Function<int>", string_getLength, false)
	ADD("getHash", "Function<int>", string_getHash, false)
END_METHODS

/*
 * Function class
 */
START_METHODS(function)
END_METHODS

/*
 * Standard global functions
 */
FUNCTION(time) {
	RETURN_NUMBER(((double) clock()) / CLOCKS_PER_SEC);
}

FUNCTION(print) {
	printf("%s\n", lit_to_string(vm, args[0]));
	RETURN_VOID
}

LitLibRegistry* lit_create_std(LitCompiler* compiler) {
	START_LIB

	START_CLASSES(8)
		DEFINE_CLASS("Class", class, NULL)
		DEFINE_CLASS("Object", object, NULL)
		DEFINE_CLASS("Bool", bool, object_class)
		DEFINE_CLASS("Int", int, object_class)
		DEFINE_CLASS("Double", double, int_class)
		DEFINE_CLASS("Char", char, object_class)
		DEFINE_CLASS("String", string, object_class)
		DEFINE_CLASS("Function", function, object_class)
	END_CLASSES

	START_FUNCTIONS(2)
		DEFINE_FUNCTION(time_native, "time", "Function<double>")
		DEFINE_FUNCTION(print_native, "print", "Function<any, void>")
	END_FUNCTIONS

	END_LIB
}