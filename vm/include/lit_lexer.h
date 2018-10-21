#ifndef LIT_LEXER_H
#define LIT_LEXER_H

#include "lit_vm.h"

typedef enum {
	TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
	TOKEN_COMMA, TOKEN_DOT, TOKEN_RANGE, TOKEN_MINUS, TOKEN_PLUS,
	TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR, TOKEN_COLLUMN,
	TOKEN_AMPERSAND, TOKEN_BAR,

	TOKEN_BANG, TOKEN_BANG_EQUAL,
	TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
	TOKEN_GREATER, TOKEN_GREATER_EQUAL,
	TOKEN_LESS, TOKEN_LESS_EQUAL,
	TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL,
	TOKEN_STAR_EQUAL, TOKEN_SLASH_EQUAL,

	TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER, TOKEN_CHAR,

	TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
	TOKEN_FUN, TOKEN_FOR, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
	TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
	TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE, TOKEN_BREAK,
	TOKEN_CONTINUE, TOKEN_SWITCH, TOKEN_ABSTRACT,
	TOKEN_OVERRIDE, TOKEN_STATIC, TOKEN_PRIVATE,
	TOKEN_PUBLIC, TOKEN_PROTECTED,

	TOKEN_ERROR,
	TOKEN_EOF
} LitTokenType;

typedef struct {
	LitTokenType type;
	const char* start;
	int length;
	int line;
} LitToken;

typedef struct {
	const char* start;
	const char* current_code;
	int line;

	LitVm* vm;

	LitToken current;
	LitToken previous;

	bool had_error;
	bool panic_mode;
	bool ended;
} LitLexer;

void lit_init_lexer(LitLexer* lexer, const char* code);
void lit_free_lexer(LitLexer* lexer);

LitToken lit_lexer_next_token(LitLexer* lexer);

#endif