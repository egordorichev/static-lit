#include <stdio.h>

#include "lit_value.h"
#include "lit_memory.h"

static char output[21];

char *lit_to_string(LitValue value) {
	if (IS_BOOL(value)) {
		return AS_BOOL(value) ? "true" : "false";
	} else if (IS_NUMBER(value)) {
		snprintf(output, 21, "%g", AS_NUMBER(value));
		output[20] = '\0';

		return output;
	} else if (IS_NIL(value)) {
		return "nil";
	} else if (IS_OBJECT(value)) {
		// TODO
	}

	UNREACHABLE()
}

bool lit_is_false(LitValue value) {
	return IS_NIL(value) || (IS_NUMBER(value) && AS_NUMBER(value) == 0)
		|| (IS_BOOL(value) && !AS_BOOL(value));
}