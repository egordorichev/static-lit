#include <stdio.h>
#include <memory.h>

#include "lit_value.h"
#include "lit_memory.h"
#include "lit_object.h"
#include "lit.h"

static char output[21];

char *lit_to_string(LitVm* vm, LitValue value) {
	if (IS_BOOL(value)) {
		return AS_BOOL(value) ? "true" : "false";
	} else if (IS_NUMBER(value)) {
		snprintf(output, 21, "%g", AS_NUMBER(value));
		output[20] = '\0';

		return output;
	} else if (IS_NIL(value)) {
		return "nil";
	} else if (IS_CHAR(value)) {
		DoubleUnion data;
		data.bits64 = value;

		snprintf(output, 2, "%c", data.bits16[0]);
		output[2] = '\0';

		return output;
	} else if (IS_OBJECT(value)) {
		switch (AS_OBJECT(value)->type) {
			case OBJECT_STRING: return AS_CSTRING(value);
			case OBJECT_NATIVE: return "<native fun>";
			case OBJECT_FUNCTION: {
				char *name = AS_FUNCTION(value)->name->chars;
				int len = strlen(name);
				char* buffer = ALLOCATE(vm, char, len + 6);

				sprintf(buffer, "<fun %s", name);
				buffer[len + 5] = '>';
				buffer[len + 6] = '\0';

				return buffer;
			}
			case OBJECT_INSTANCE: {
				char *name = AS_INSTANCE(value)->type->name->chars;
				int len = strlen(name);
				char* buffer = ALLOCATE(vm, char, len + 10);

				sprintf(buffer, "instance %s", name);
				buffer[len + 10] = '\0';

				return buffer;
			}
			case OBJECT_CLASS: {
				char *name = AS_CLASS(value)->name->chars;
				int len = strlen(name);
				char* buffer = ALLOCATE(vm, char, len + 7);

				sprintf(buffer, "class %s", name);
				buffer[len + 7] = '\0';

				return buffer;
			}
			case OBJECT_UPVALUE: return "upvalue";
			case OBJECT_BOUND_METHOD: {
				char *name = AS_METHOD(value)->method->function->name->chars;
				int len = strlen(name);
				char* buffer = ALLOCATE(vm, char, len + 6);

				sprintf(buffer, "<fun %s", name);
				buffer[len + 5] = '>';
				buffer[len + 6] = '\0';

				return buffer;
			}
			case OBJECT_CLOSURE: {
				char *name = AS_CLOSURE(value)->function->name->chars;
				int len = strlen(name);
				char* buffer = ALLOCATE(vm, char, len + 6);

				sprintf(buffer, "<fun %s", name);
				buffer[len + 5] = '>';
				buffer[len + 6] = '\0';

				return buffer;
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