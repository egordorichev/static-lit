#include <string.h>
#include <lit_lexer.h>
#include <stdio.h>
#include "lit_lexer.h"
#include "lit_common.h"

static bool is_digit(char c) {
	return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static inline bool is_at_end(LitLexer* lexer) {
	return *lexer->current_code == '\0' || lexer->ended;
}

static LitToken make_token(LitLexer* lexer, LitTokenType type) {
	LitToken token;

	token.type = type;
	token.start = lexer->start;
	token.length = (int) (lexer->current_code - lexer->start);
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
	lexer->current_code++;
	return lexer->current_code[-1];
}

static bool match(LitLexer* lexer, char expected) {
	if (is_at_end(lexer)) {
		return false;
	}

	if (*lexer->current_code != expected) {
		return false;
	}

	lexer->current_code++;
	return true;
}

static char peek(LitLexer* lexer) {
	return *lexer->current_code;
}

static char peek_next(LitLexer* lexer) {
	if (is_at_end(lexer)) {
		return '\0';
	}

	return lexer->current_code[1];
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
					advance(lexer);

					if (peek_next(lexer) == '*') {
						lexer->ended = true;
						return;
					}

					while (peek(lexer) != '\n' && !is_at_end(lexer)) {
						advance(lexer);
					}
				} else if (peek_next(lexer) == '*') {
					advance(lexer);
					advance(lexer);

					while ((peek(lexer) != '*' || peek_next(lexer) != '/') && !is_at_end(lexer)) {
						if (peek(lexer) == '\n') {
							lexer->line++;
						}

						advance(lexer);
					}

					advance(lexer); // Star
					advance(lexer); // Slash
				} else {
					return;
				}

				break;
			default:
				return;
		}
	}
}

static LitTokenType check_keyword(LitLexer* lexer, int start, int length, const char* rest, LitTokenType type) {
	if (lexer->current_code - lexer->start == start + length &&
	    memcmp(lexer->start + start, rest, length) == 0) {

		return type;
	}

	return TOKEN_IDENTIFIER;
}

static LitTokenType find_identifier_type(LitLexer* lexer) {
	switch (lexer->start[0]) {
		case 'a': return check_keyword(lexer, 1, 7, "bstract", TOKEN_ABSTRACT);
		case 'b': return check_keyword(lexer, 1, 4, "reak", TOKEN_BREAK);
		case 'c': {
			if (lexer->current_code - lexer->start > 1) {
				switch (lexer->start[1]) {
					case 'l': return check_keyword(lexer, 2, 3, "ass", TOKEN_CLASS);
					case 'o': return check_keyword(lexer, 2, 6, "ntinue", TOKEN_CONTINUE);
				}
			}

			break;
		}
		case 'e': return check_keyword(lexer, 1, 3, "lse", TOKEN_ELSE);
		case 'i': return check_keyword(lexer, 1, 1, "f", TOKEN_IF);
		case 'n': return check_keyword(lexer, 1, 2, "il", TOKEN_NIL);
		case 'r': return check_keyword(lexer, 1, 5, "eturn", TOKEN_RETURN);
		case 's': {
			if (lexer->current_code - lexer->start > 1) {
				switch (lexer->start[1]) {
					case 'u': return check_keyword(lexer, 2, 3, "per", TOKEN_SUPER);
					case 'w': return check_keyword(lexer, 2, 4, "itch", TOKEN_SWITCH);
					case 't': return check_keyword(lexer, 2, 4, "atic", TOKEN_STATIC);
				}
			}

			break;
		}
		case 'v': return check_keyword(lexer, 1, 2, "ar", TOKEN_VAR);
		case 'w': return check_keyword(lexer, 1, 4, "hile", TOKEN_WHILE);
		case 'f': {
			if (lexer->current_code - lexer->start > 1) {
				switch (lexer->start[1]) {
					case 'a': return check_keyword(lexer, 2, 3, "lse", TOKEN_FALSE);
					case 'o': return check_keyword(lexer, 2, 1, "r", TOKEN_FOR);
					case 'u': return check_keyword(lexer, 2, 1, "n", TOKEN_FUN);
					case 'i': return check_keyword(lexer, 2, 3, "nal", TOKEN_FINAL);
				}
			}

			break;
		}
		case 't': {
			if (lexer->current_code - lexer->start > 1) {
				switch (lexer->start[1]) {
					case 'h': return check_keyword(lexer, 2, 2, "is", TOKEN_THIS);
					case 'r': return check_keyword(lexer, 2, 2, "ue", TOKEN_TRUE);
				}
			}

			break;
		}
		case 'o': return check_keyword(lexer, 1, 7, "verride", TOKEN_OVERRIDE);
		case 'p': {
			if (lexer->current_code - lexer->start > 1) {
				switch (lexer->start[1]) {
					case 'u': return check_keyword(lexer, 2, 4, "blic", TOKEN_PUBLIC);
					case 'r': {
						if (lexer->current_code - lexer->start > 2) {
							switch (lexer->start[2]) {
								case 'i': return check_keyword(lexer, 3, 4, "vate", TOKEN_PRIVATE);
								case 'o': return check_keyword(lexer, 3, 6, "tected", TOKEN_PROTECTED);
							}
						}

						break;
					}
				}
			}

			break;
		}
	}

	return TOKEN_IDENTIFIER;
}

LitToken lit_lexer_next_token(LitLexer* lexer) {
	skip_whitespace(lexer);

	lexer->start = lexer->current_code;

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

		return make_token(lexer, find_identifier_type(lexer));
	}

	switch (c) {
		case '(': return make_token(lexer, TOKEN_LEFT_PAREN);
		case '&': return make_token(lexer, match(lexer, '&') ? TOKEN_AND : TOKEN_AMPERSAND);
		case '|': return make_token(lexer, match(lexer, '|') ? TOKEN_OR : TOKEN_BAR);
		case ')': return make_token(lexer, TOKEN_RIGHT_PAREN);
		case '{': return make_token(lexer, TOKEN_LEFT_BRACE);
		case '}': return make_token(lexer, TOKEN_RIGHT_BRACE);
		case ';': return make_token(lexer, TOKEN_SEMICOLON);
		case ',': return make_token(lexer, TOKEN_COMMA);
		case '.': return make_token(lexer, match(lexer, '.') ? TOKEN_RANGE : TOKEN_DOT);
		case '-': return make_token(lexer, match(lexer, '=') ? TOKEN_MINUS_EQUAL : TOKEN_MINUS);
		case '+': return make_token(lexer, match(lexer, '=') ? TOKEN_PLUS_EQUAL : TOKEN_PLUS);
		case '/': return make_token(lexer, match(lexer, '=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH);
		case '*': return make_token(lexer, match(lexer, '=') ? TOKEN_STAR_EQUAL : TOKEN_STAR);
		case ':': return make_token(lexer, TOKEN_COLLUMN);
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
		case '\'': {
			if (peek(lexer) == '\'') {
				return make_error_token(lexer, "Empty char literal");
			}

			advance(lexer); // Consume the char

			if (!match(lexer, '\'')) {
				return make_error_token(lexer, "Expected '\'' after char literal");
			}

			return make_token(lexer, TOKEN_CHAR);
		}
	}

	return make_error_token(lexer, "Unexpected character");
}

void lit_init_lexer(LitLexer* lexer, const char* code) {
	lexer->start = code;
	lexer->current_code = code;
	lexer->line = 1;
	lexer->had_error = false;
	lexer->panic_mode = false;
	lexer->vm = NULL;
	lexer->ended = false;
}

void lit_free_lexer(LitLexer* lexer) {

}