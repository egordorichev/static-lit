#ifndef LIT_CONTEXT_HPP
#define LIT_CONTEXT_HPP

#include "lit_lexer.hpp"
#include "lit_compiler.hpp"
#include "lit_vm.hpp"

class LitContext {
public:
  LitContext();

  InterpretResult execute(const char* string);

private:
  LitLexer lexer;
  LitCompiler compiler;
  LitVm vm;
};

#endif