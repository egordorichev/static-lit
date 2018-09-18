#ifndef LIT_COMPILER_H
#define LIT_COMPILER_H

#include "lit_chunk.h"
#include "lit_common.h"

typedef enum {
	TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
	TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
	TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

	TOKEN_BANG, TOKEN_BANG_EQUAL,
	TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
	TOKEN_GREATER, TOKEN_GREATER_EQUAL,
	TOKEN_LESS, TOKEN_LESS_EQUAL,

	TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

	TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
	TOKEN_FUN, TOKEN_FOR, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
	TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
	TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

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
	const char* current;
	int line;

	LitVm* vm;
} LitLexer;

void lit_init_lexer(LitLexer* lexer, const char* code);
void lit_free_lexer(LitLexer* lexer);

// FIXME: split lexer and compiler into separate files

typedef struct {
	LitLexer lexer;
	LitChunk* chunk;
	LitVm* vm;
} LitCompiler;

void lit_init_compiler(LitCompiler* compiler);
void lit_free_compiler(LitCompiler* compiler);

bool lit_compile(LitCompiler* compiler, const char* code);

#endif