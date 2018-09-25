#ifndef LIT_COMPILER_H
#define LIT_COMPILER_H

#include "lit_chunk.h"
#include "lit_common.h"
#include "lit_lexer.h"
#include "lit_object.h"

typedef struct {
	LitToken name;
	int depth;
	bool upvalue;
} LitLocal;

typedef struct {
	uint8_t index;
	bool local;
} LitCompilerUpvalue;

typedef enum {
	TYPE_FUNCTION,
	TYPE_INITIALIZER,
	TYPE_METHOD,
	TYPE_TOP_LEVEL
} LitFunctionType;

typedef struct sLitLoop {
	int start;
	int exit_jump;
	int body;
	int depth;
	struct sLitLoop* enclosing;
} LitLoop;

typedef struct LitClassCompiler {
	struct LitClassCompiler* enclosing;

	LitToken name;
	bool has_super;
} LitClassCompiler;

typedef struct sLitCompiler {
	struct sLitCompiler* enclosing;
	LitLexer lexer;
	LitVm* vm;
	LitFunction* function;
	LitFunctionType type;
	LitLoop *loop;

	LitCompilerUpvalue upvalues[UINT8_COUNT];
	LitLocal locals[UINT8_COUNT];

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

typedef void (*LitParseFn)(LitCompiler* compiler, bool can_assign);

typedef struct {
	LitParseFn prefix;
	LitParseFn infix;
	LitPrecedence precedence;
} LitParseRule;

void lit_init_compiler(LitVm* vm, LitCompiler* compiler, LitCompiler* enclosing, LitFunctionType type);
void lit_free_compiler(LitCompiler* compiler);

LitFunction *lit_compile(LitVm* vm, const char* code);

#endif