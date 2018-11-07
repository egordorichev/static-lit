#include <memory.h>

#include <compiler/lit_compiler.h>
#include <compiler/lit_parser.h>
#include <compiler/lit_emitter.h>

void lit_init_compiler(LitCompiler* compiler) {
	LitMemManager* manager = (LitMemManager*) compiler;

	manager->bytes_allocated = 0;
	manager->type = MANAGER_COMPILER;

	compiler->resolver.compiler = compiler;
	lit_init_resolver(&compiler->resolver);
	lit_init_letals(&compiler->resolver.externals);
}

void lit_free_compiler(LitCompiler* compiler) {
	lit_free_resolver(&compiler->resolver);
}

/*
 * Splits source code into tokens and converts them to AST tree
 * Then resolves the AST tree (finds non existing vars, etc)
 * And emits it into bytecode
 */

LitChunk* lit_compile(LitCompiler* compiler, const char* source_code) {
	lit_init_lexer(compiler, &compiler->lexer, source_code);
	LitStatements statements;

	lit_init_statements(&statements);
	lit_parse(compiler, &compiler->lexer, &statements);

	if (!lit_resolve(compiler, &statements)) {
		return NULL; // Resolving error
	}

	LitChunk* chunk = lit_emit(compiler, &statements);
	lit_free_statements((LitMemManager *) compiler, &statements);

	return chunk;
}