#ifndef LIT_CONTEXT_HPP
#define LIT_CONTEXT_HPP

#include "lit_lexer.hpp"
#include "lit_compiler.hpp"
#include "lit_vm.hpp"

class LitContext {
	private:
		LitLexer lexer;
		LitCompiler compiler;
		LitVm vm;

	public:
	  LitContext();

	  LitInterpretResult execute(const char* string);
};

#endif