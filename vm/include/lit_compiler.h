#ifndef LIT_COMPILER_H
#define LIT_COMPILER_H

#include "lit_chunk.h"
#include "lit_common.h"
#include "lit_lexer.h"

typedef struct {
	LitLexer lexer;
	LitChunk* chunk;
	LitVm* vm;
} LitCompiler;

void lit_init_compiler(LitCompiler* compiler);
void lit_free_compiler(LitCompiler* compiler);

bool lit_compile(LitCompiler* compiler, const char* code);

#endif