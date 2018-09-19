#ifndef LIT_COMPILER_H
#define LIT_COMPILER_H

#include "lit_chunk.h"
#include "lit_common.h"
#include "lit_lexer.h"

typedef struct {
	LitToken name;
	int depth;
	bool upvalue;
} LitLocal;

typedef struct {
	LitLexer lexer;
	LitChunk* chunk;
	LitVm* vm;

	LitLocal locals[UINT8_MAX + 1];
	int local_count;
	int depth;
} LitCompiler;

typedef enum {
	PREC_NONE,
	PREC_ASSIGNMENT,  // =
	PREC_OR,          // or
	PREC_AND,         // and
	PREC_EQUALITY,    // == !=
	PREC_COMPARISON,  // < > <= >=
	PREC_TERM,        // + -
	PREC_FACTOR,      // * /
	PREC_UNARY,       // ! - +
	PREC_CALL,        // . () []
	PREC_PRIMARY
} LitPrecedence;

typedef void (*LitParseFn)(LitCompiler* compiler);

typedef struct {
	LitParseFn prefix;
	LitParseFn infix;
	LitPrecedence precedence;
} LitParseRule;

void lit_init_compiler(LitCompiler* compiler);
void lit_free_compiler(LitCompiler* compiler);

bool lit_compile(LitCompiler* compiler, LitChunk* chunk, const char* code);

#endif