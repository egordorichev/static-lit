#include "lit_compiler.h"
#include "lit_vm.h"

/*
 * Lexer implementation
 */

void lit_init_lexer(LitLexer* lexer, const char* code) {
	lexer->start = code;
	lexer->current = code;
	lexer->line = 1;
}

void lit_free_lexer(LitLexer* lexer) {

}

/*
 * Compiler implementation
 */

void lit_init_compiler(LitCompiler* compiler) {

}

void lit_free_compiler(LitCompiler* compiler) {
	lit_free_lexer(&compiler->lexer);
}

bool lit_compile(LitCompiler* compiler, const char* code) {
	compiler->lexer.vm = compiler->vm;
	lit_init_lexer(&compiler->lexer, code);
}