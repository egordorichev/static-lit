#include <std/lit_std.h>
#include <time.h>
#include <ctype.h>

/*
 * Class metaclass
 */
START_METHODS(class)
END_METHODS

/*
 * Object class
 */
METHOD(object_test) {
	printf("Test method was called!\n");
	RETURN_VOID
}

START_METHODS(object)
	ADD("test", "Function<void>", object_test, false)
END_METHODS

/*
 * Bool class
 */
START_METHODS(bool)
END_METHODS

/*
 * Int class
 */
START_METHODS(int)
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
	LitString* old = AS_STRING(MAKE_OBJECT_VALUE(instance));
	LitString* string = lit_new_string(vm, old->length);

	for (int i = 0; i < old->length; i++) {
		string->chars[i] = (char) tolower(old->chars[i]);
	}

	string->chars[old->length] = '\0';

	lit_hash_string(string);
	RETURN_STRING(string)
}

START_METHODS(string)
	ADD("toLowerCase", "Function<void>", string_toLowerCase, false)
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