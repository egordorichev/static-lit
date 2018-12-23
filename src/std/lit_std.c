#include <lit_bindings.h>
#include <std/lit_std.h>

#include <time.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

/*
 * Class metaclass
 */
START_METHODS(class)
END_METHODS

/*
 * Object class
 */
START_METHODS(object)
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
START_METHODS(string)
END_METHODS

/*
 * Function class
 */
START_METHODS(function)
END_METHODS

/*
 * Standard global functions
 */
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

	START_FUNCTIONS(0)
	END_FUNCTIONS

	END_LIB
}