#ifndef LIT_COMPILER_HPP
#define LIT_COMPILER_HPP

#include "lit_lexer.hpp"
#include "lit_chunk.hpp"

class LitCompiler {
	public:
		void set_lexer(LitLexer *lexer) { this->lexer = lexer; };
		bool compile(LitChunk *cnk);
	private:
		void advance();
		void parse_expression();
		void consume(LitTokenType token, const char *error);

		LitChunk *chunk;
		LitLexer *lexer;
		LitToken current;
		LitToken previous;
};

#endif