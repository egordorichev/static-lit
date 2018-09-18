#include <string.h>
#include "lit_lexer.h"
#include "lit_common.h"

void lit_init_lexer(LitLexer* lexer, const char* code) {
	lexer->start = code;
	lexer->current = code;
	lexer->line = 1;
}

void lit_free_lexer(LitLexer* lexer) {

}

static bool is_digit(char c) {
	return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static inline bool is_at_end(LitLexer* lexer) {
	return *lexer->current == '\0';
}

static LitToken make_token(LitLexer* lexer, LitTokenType type) {
	LitToken token;

	token.type = type;
	token.start = lexer->start;
	token.length = (int) (lexer->current - lexer->start);
	token.line = lexer->line;

	return token;
}

static LitToken make_error_token(LitLexer* lexer, const char* message) {
	LitToken token;

	token.type = TOKEN_ERROR;
	token.start = message;
	token.length = (int) strlen(message);
	token.line = lexer->line;

	return token;
}

static char advance(LitLexer* lexer) {
	lexer->current++;
	return lexer.current[-1];
}

static bool match(LitLexer* lexer, char expected) {
	if (is_at_end(lexer)) {
		return false;
	}

	if (*lexer->current != expected) {
		return false;
	}

	lexer->current++;
	return true;
}

static char peek(LitLexer* lexer) {
	return *lexer->current;
}

static char peek_next(LitLexer* lexer) {
	if (is_at_end(lexer)) {
		return '\0';
	}

	return lexer->current[1];
}

static void skip_whitespace(LitLexer* lexer) {
	for (;;) {
		char c = peek(lexer);

		switch (c) {
			case ' ':
			case '\r':
			case '\t':
				advance(lexer);
				break;
			case '\n':
				lexer->line++;
				advance(lexer);
				break;
			case '/':
				if (peek_next(lexer) == '/') {
					while (peek(lexer) != '\n' && !is_at_end(lexer)) {
						advance(lexer);
					}
				} else {
					return;
				}
				break;
			default:
				return;
		}
	}
}

LitToken lit_lexer_next_token(LitLexer* lexer) {
	skip_whitespace(lexer);

	lexer->start = lexer->current;

	if (is_at_end(lexer)) {
		return make_token(lexer, TOKEN_EOF);
	}

	char c = advance(lexer);

	if (is_digit(c)) {
		while (is_digit(peek(lexer))) {
			advance(lexer);
		}

		if (peek(lexer) == '.' && is_digit(peek_next(lexer))) {
			advance(lexer);

			while (is_digit(peek(lexer))) {
				advance(lexer);
			}
		}

		return make_token(lexer, TOKEN_NUMBER);
	}

	if (is_alpha(c)) {
		while (is_alpha(peek(lexer)) || is_digit(peek(lexer))) {
			advance(lexer);
		}

		return make_token(lexer, find_identifier_type());
	}

	switch (c) {
		case '(': return make_token(lexer, TOKEN_LEFT_PAREN);
		case ')': return make_token(lexer, TOKEN_RIGHT_PAREN);
		case '{': return make_token(lexer, TOKEN_LEFT_BRACE);
		case '}': return make_token(lexer, TOKEN_RIGHT_BRACE);
		case ';': return make_token(lexer, TOKEN_SEMICOLON);
		case ',': return make_token(lexer, TOKEN_COMMA);
		case '.': return make_token(lexer, TOKEN_DOT);
		case '-': return make_token(lexer, TOKEN_MINUS);
		case '+': return make_token(lexer, TOKEN_PLUS);
		case '/': return make_token(lexer, TOKEN_SLASH);
		case '*': return make_token(lexer, TOKEN_STAR);
		case '!': return make_token(lexer, match(lexer, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
		case '=': return make_token(lexer, match(lexer, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
		case '<': return make_token(lexer, match(lexer, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
		case '>': return make_token(lexer, match(lexer, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
		case '"': {
			while (peek(lexer) != '"' && !is_at_end(lexer)) {
				if (peek(lexer) == '\n') {
					lexer->line++;
				}

				advance(lexer);
			}

			if (is_at_end(lexer)) {
				return make_error_token(lexer, "Unterminated string");
			}

			advance(lexer);
			return make_token(lexer, TOKEN_STRING);
		}
		case
	}

	return make_error_token(lexer, "Unexpected character");
}