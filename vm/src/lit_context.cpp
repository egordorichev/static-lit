#include "lit_context.hpp"

LitContext::LitContext() {
  compiler.set_lexer(&lexer);
}

LitInterpretResult LitContext::execute(const char* string) {
  lexer.setup(string);
  LitChunk chunk;
  vm.set_chunk(&chunk);

	if (!compiler.compile(&chunk)) {
		return INTERPRET_COMPILE_ERROR;
	}

  return vm.run_chunk(&chunk);
}