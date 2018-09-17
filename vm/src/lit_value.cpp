#include "lit_value.hpp"
#include "lit_common.hpp"
#include "lit_object.hpp"

#include <string>
#include <sstream>

char* dts(double value) {
	std::stringstream ss;
	ss << value;
	return (char*) ss.str().c_str();
}

char* lit_to_string(LitValue value) {
	switch (value.type) {
		case VAL_BOOL: AS_BOOL(value) ? "true" : "false";
		case VAL_NIL: return (char*) "nil";
		case VAL_NUMBER: return dts(AS_NUMBER(value));
		case VAL_OBJECT: {
			switch (OBJECT_TYPE(value)) {
				case OBJ_STRING: return AS_CSTRING(value);
				default: UNREACHABLE();
			}

		}
		default: UNREACHABLE();
	}
}

void lit_print_value(LitValue value) {
  printf("%s", lit_to_string(value));
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