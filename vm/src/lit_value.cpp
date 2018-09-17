#include "lit_value.hpp"
#include "lit_common.hpp"
#include "lit_object.hpp"

void lit_print_value(LitValue value) {
  switch (value.type) {
    case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
    case VAL_NIL: printf("nil"); break;
    case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
    case VAL_OBJECT: {
	    switch (OBJECT_TYPE(value)) {
		    case OBJ_STRING: printf("%s", AS_CSTRING(value)); break;
	    }

	    break;
    }
    default: UNREACHABLE();
  }
}

bool lit_values_are_equal(LitValue a, LitValue b) {
	if (a.type != b.type) {
		return false;
	}

	switch (a.type) {
		case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
		case VAL_NIL: return true;
		case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
		default: UNREACHABLE();
	}
}