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
} LitEmitterFunction;

DECLARE_ARRAY(LitInts, uint64_t, ints)
DECLARE_ARRAY(LitShorts, uint16_t, shorts)

typedef struct LitEmitter {
	LitEmitterFunction* function;
	LitClassCompiler* class;
	LitCompiler* compiler;
	LitInts breaks;
	uint16_t register_count;
	LitShorts free_registers;

	uint64_t loop_start;
	bool had_error;
} LitEmitter;

void lit_init_emitter(LitCompiler* compiler, LitEmitter* emitter);
void lit_free_emitter(LitEmitter* emitter);
LitFunction* lit_emit(LitEmitter* emitter, LitStatements* statements);

#endif