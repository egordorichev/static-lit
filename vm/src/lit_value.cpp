#include <string>

#include "lit_value.hpp"
#include "lit_common.hpp"
#include "lit_object.hpp"

char buff[21];

char* dts(LitValue value) {
	sprintf(buff, "%g", (double) value);
	return buff;
}

const char* lit_to_string(LitValue value) {
	if (IS_BOOL(value)) {
		return AS_BOOL(value) == 0 ? "false" : "true";
	} else if (IS_NIL(value)) {
		return "nil";
	} else if (IS_NUMBER(value)) {
		return dts(AS_NUMBER(value));
	} else if (IS_OBJECT(value)) {
		switch (OBJECT_TYPE(value)) {
			case OBJ_STRING: return AS_CSTRING(value);
			default: UNREACHABLE();
		}
	} else {
		UNREACHABLE()
	}
}

void lit_print_value(LitValue value) {
  printf("%s", lit_to_string(value));
}