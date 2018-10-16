#ifndef LIT_CORE_H
#define LIT_CORE_H

#include "lit_vm.h"
#include "lit_object.h"
#include "lit_value.h"

void lit_init_core(LitVm* vm);
LitClass* lit_define_class(LitVm* vm, const char* name);

#endif