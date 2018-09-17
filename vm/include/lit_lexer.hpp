#ifndef LIT_SCANNER_HPP
#define LIT_SCANNER_HPP

#include "lit_config.hpp"

enum LitTokenType {
	// Single-character tokens
	TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
	TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
	TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

	// One or two character tokens
	TOKEN_BANG, TOKEN_BANG_EQUAL,
	TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
	TOKEN_GREATER, TOKEN_GREATER_EQUAL,
	TOKEN_LESS, TOKEN_LESS_EQUAL,

	// Literals
	TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

	// Keywords
	TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
	TOKEN_FUN, TOKEN_FOR, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
	TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
	TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

	TOKEN_ERROR,
	TOKEN_EOF
};

struct LitToken {
	LitTokenType type;
	const char* start;
	int length;
	int line;
};

class LitLexer {
	public:
		LitLexer();

		void setup(const char *source);
		LitToken next_token();
		LitToken make_token(LitTokenType type);
		LitToken make_error_token(const char *error);

		bool is_at_end() { return *current == '\0'; }
	private:
		char advance();
		bool match(char next);
		void skip_whitespace();
		LitTokenType parse_identifier_type();
		LitTokenType check_keyword(int start, int length, const char* rest, LitTokenType type);
		char peek();
		char peek_next();

		const char* start;
		const char* current;
		int line;
};

#endif