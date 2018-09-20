#ifndef LIT_VM_H
#define LIT_VM_H

#include "lit_common.h"
#include "lit_value.h"

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
} LitInterpretResult;

typedef struct _LitVm LitVm;

#endif