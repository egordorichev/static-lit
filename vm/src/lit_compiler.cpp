#include "lit_compiler.hpp"

void LitCompiler::advance() {

}

void LitCompiler::parse_expression() {

}

void LitCompiler::consume(LitTokenType token, const char *error) {
	
}

bool LitCompiler::compile(LitChunk *cnk) {
	chunk = cnk;

	advance();
	parse_expression();
	consume(TOKEN_EOF, "Expect end of expression.");
}