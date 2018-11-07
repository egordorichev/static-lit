#include <memory.h>

#include <lit_debug.h>

#include <compiler/lit_compiler.h>
#include <compiler/lit_parser.h>
#include <compiler/lit_emitter.h>

void lit_init_compiler(LitCompiler* compiler) {
	LitMemManager* manager = (LitMemManager*) compiler;

	manager->bytes_allocated = 0;
	manager->type = MANAGER_COMPILER;
	manager->objects = NULL;

	lit_init_table(&manager->strings);

	compiler->resolver.compiler = compiler;
	lit_init_resolver(&compiler->resolver);
	lit_init_letals(&compiler->resolver.externals);
}

void lit_free_compiler(LitCompiler* compiler) {
	LitMemManager* manager = (LitMemManager*) compiler;
	lit_free_table(compiler, &manager->strings);

	lit_free_resolver(&compiler->resolver);
}

/*
 * Splits source code into tokens and converts them to AST tree
 * Then resolves the AST tree (finds non existing vars, etc)
 * And emits it into bytecode
 */

LitFunction* lit_compile(LitCompiler* compiler, const char* source_code) {
	lit_init_lexer(compiler, &compiler->lexer, source_code);
	LitStatements statements;

	lit_init_statements(&statements);

	if (lit_parse(compiler, &compiler->lexer, &statements)) {
		for (int i = 0; i < statements.count; i++) {
			lit_free_statement(compiler, statements.values[i]);
		}

		lit_free_statements((LitMemManager *) compiler, &statements);
		return NULL; // Parsing error
	}

	if (DEBUG_TRACE_AST) {
		printf("[\n");

		for (int i = 0; i < statements.count; i++) {
			lit_trace_statement(compiler, statements.values[i], 1);

			if (i < statements.count - 1) {
				printf(",\n");
			}
		}

		printf("\n]\n");
	}

	if (lit_resolve(compiler, &statements)) {
		for (int i = 0; i < statements.count; i++) {
			lit_free_statement(compiler, statements.values[i]);
		}

		lit_free_statements((LitMemManager *) compiler, &statements);
		return NULL; // Resolving error
	}

	LitFunction* function = lit_emit(compiler, &statements);

	if (DEBUG_TRACE_CODE) {
		lit_trace_chunk(compiler, &function->chunk, "top-level");
	}

	for (int i = 0; i < statements.count; i++) {
		lit_free_statement(compiler, statements.values[i]);
	}

	lit_free_statements((LitMemManager *) compiler, &statements);
	return function;
}