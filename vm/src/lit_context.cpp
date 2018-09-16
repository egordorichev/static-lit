#include "lit_context.hpp"

LitContext::LitContext() {
	compiler.set_lexer(&lexer);
}

InterpretResult LitContext::execute(const char *string) {
	lexer.setup(string);
	LitChunk chunk;

	if (!compiler.compile(&chunk)) {
		return INTERPRET_COMPILE_ERROR;
	}

	return INTERPRET_OK;
}