#include <stdio.h>
#include <memory.h>

#include <vm/lit_value.h>
#include <vm/lit_memory.h>
#include <vm/lit_object.h>
#include <lit.h>

// FIXME: move to non global
static char output[21];
static DoubleUnion data;

char *lit_to_string(LitVm* vm, LitValue value) {
	int tag = GET_TAG(value);

	if (tag == TAG_FALSE) {
		return "false";
	} else if (tag == TAG_TRUE) {
		return "true";
	} else if (tag == TAG_CHAR) {
		data.bits64 = value;

		snprintf(output, 2, "%c", data.bits16[0]);
		output[2] = '\0';

		return output;
	} else if (tag == TAG_NIL) {
		return "nil";
	} else if (IS_NUMBER(value)) {
		snprintf(output, 21, "%g", AS_NUMBER(value));
		output[20] = '\0';

		return output;
	} else if (IS_OBJECT(value)) {
		switch (AS_OBJECT(value)->type) {
			case OBJECT_STRING: {
				return AS_CSTRING(value);
			}
			case OBJECT_NATIVE: {
				return "<native function>"; // FIXME: get name somehow?
			}
			case OBJECT_NATIVE_METHOD: {
				return "<native method>"; // FIXME: get name somehow?
			}
			case OBJECT_FUNCTION: {
				return lit_format_string(vm, "<fun %>", AS_FUNCTION(value)->name)->chars;
			}
			case OBJECT_INSTANCE: {
				return lit_format_string(vm, "% instance", AS_INSTANCE(value)->type->name)->chars;
			}
			case OBJECT_CLASS: {
				return lit_format_string(vm, "Class<%>", AS_CLASS(value)->name)->chars;
			}
			case OBJECT_UPVALUE: {
				return lit_to_string(vm, *AS_UPVALUE(value)->value);
			}
			case OBJECT_BOUND_METHOD: {
				return lit_format_string(vm, "<method %>", AS_METHOD(value)->method->function->name)->chars;
			}
			case OBJECT_CLOSURE: {
				return lit_format_string(vm, "<function %>", AS_CLOSURE(value)->function->name)->chars;
			}
			default: UNREACHABLE();
		}
	}

	UNREACHABLE()
	return NULL; // To make gcc shut up, never actually reached
}

bool lit_is_false(LitValue value) {
	return IS_NIL(value) || (IS_NUMBER(value) && AS_NUMBER(value) == 0)
		|| (IS_BOOL(value) && !AS_BOOL(value));
}

bool lit_are_values_equal(LitValue a, LitValue b) {
	return a == b;
}