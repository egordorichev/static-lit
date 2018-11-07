#ifndef LIT_EMITTER_H
#define LIT_EMITTER_H

#include <vm/lit_object.h>
#include <compiler/lit_ast.h>

typedef struct LitLocal {
	const char* name;
	int depth;
	bool upvalue;
} LitLocal;

typedef struct LitEmvalue {
	uint8_t index;
	bool local;
} LitEmvalue;

typedef struct LitClassCompiler {
	struct LitClassCompiler* enclosing;

	char* name;
	bool has_super;
} LitClassCompiler;

typedef struct LitEmitterFunction {
	LitFunction* function;
	struct LitEmitterFunction* enclosing;

	int local_count;
	int depth;

	LitLocal locals[UINT8_COUNT];
	LitEmvalue upvalues[UINT8_COUNT];
} LitEmitterFunction;

typedef struct LitEmitter {
	LitEmitterFunction* function;
	LitClassCompiler* class;
	LitCompiler* compiler;

	bool had_error;
} LitEmitter;

LitChunk* lit_emit(LitCompiler* compiler, LitStatements* statements);

#endif