#include <string.h>

#include "lit_core.h"

void lit_init_core(LitVm* vm) {

}

LitClass* lit_define_class(LitVm* vm, const char* name) {
	LitString* name_string = lit_copy_string(vm, name, strlen(name));
	LitClass* class = lit_new_class(vm, name_string, NULL);

	// wrenDefineVariable(vm, module, name, nameString->length, OBJ_VAL(classObj));

	return class;
}