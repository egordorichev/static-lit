#ifndef LIT_EMITTER_H
#define LIT_EMITTER_H

#include "lit_object.h"
#include "lit_ast.h"

typedef struct LitEmitter {
	int depth;
	LitFunction* function;
	LitVm* vm;
} LitEmitter;

LitFunction* lit_emit(LitVm* vm, LitStatements* statements);

#endif