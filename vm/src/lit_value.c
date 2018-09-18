#include <stdio.h>

#include "lit_value.h"
#include "lit_memory.h"


char output[21];
char *lit_to_string(LitValue value) {
	snprintf(output, 21, "%g", value);
	output[20] = '\0';
	return output;
}