#include "lit_lexer.hpp"

#include <cstring>

LitLexer::LitLexer() {

}

void LitLexer::setup(const char *source) {
	start = source;
	current = source;
	line = 1;
}

LitToken LitLexer::make_token(LitTokenType type) {
	LitToken token;
	token.type = type;
	token.start = start;
	token.length = (int) (current - start);
	token.line = line;

	return token;
}

LitToken LitLexer::make_error_token(const char *error) {
	LitToken token;
	token.type = TOKEN_ERROR;
	token.start = error;
	token.length = strlen(error);
	token.line = line;

	return token;
}

char LitLexer::advance() {
	current++;
	return current[-1];
}

bool LitLexer::match(char next) {
	if (is_at_end()) {
		return false;
	}

	if (*current != next) {
		return false;
	}

	current++;
	return true;
}

void LitLexer::skip_whitespace() {
	for (;;) {
		char c = peek();
		switch (c) {
			case ' ':
			case '\r':
			case '\t':
				advance();
				break;
			case '\n':
				line++;
				advance();
				break;
			default:
				return;
		}
	}
}

char LitLexer::peek() {
	return *current;
}

char LitLexer::peek_next() {
	if (is_at_end()) {
		return '\0';
	}

	return current[1];
}

static bool is_digit(char c) {
	return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

LitTokenType LitLexer::check_keyword(int start, int length, const char* rest, LitTokenType type) {
	if ((intptr_t) current - start == (start + length) && memcmp((char *) (start + start), rest, length) == 0) {
		return type;
	}

	return TOKEN_IDENTIFIER;
}

LitTokenType LitLexer::parse_identifier_type() {
	switch (start[0]) {
		case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
		case 'c': return check_keyword(1, 4, "lass", TOKEN_CLASS);
		case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
		case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
		case 'n': return check_keyword(1, 2, "il", TOKEN_NIL);
		case 'o': return check_keyword(1, 1, "r", TOKEN_OR);
		case 'p': return check_keyword(1, 4, "rint", TOKEN_PRINT);
		case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
		case 's': return check_keyword(1, 4, "uper", TOKEN_SUPER);
		case 'v': return check_keyword(1, 2, "ar", TOKEN_VAR);
		case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
		case 'f':
			if (current - start > 1) {
				switch (start[1]) {
					case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
					case 'o': return check_keyword(2, 1, "r", TOKEN_FOR);
					case 'u': return check_keyword(2, 1, "n", TOKEN_FUN);
				}
			}
			break;
		case 't':
			if (current - start > 1) {
				switch (start[1]) {
					case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
					case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
				}
			}
			break;
	}

	return TOKEN_IDENTIFIER;
}

LitToken LitLexer::next_token() {
	start = current;

	if (is_at_end()) {
		return make_token(TOKEN_EOF);
	}

	char c = advance();

	if (is_alpha(c)) {
		while (is_alpha(peek()) || is_digit(peek())) {
			advance();
		}

		return make_token(parse_identifier_type());
	}

	if (is_digit(c)) {
		while (is_digit(peek())) {
			advance();
		}

		if (peek() == '.' && is_digit(peek_next())) {
			advance();

			while (is_digit(peek())) {
				advance();
			}
		}

		return make_token(TOKEN_NUMBER);
	}

	switch (c) {
		case '(': return make_token(TOKEN_LEFT_PAREN);
		case ')': return make_token(TOKEN_RIGHT_PAREN);
		case '{': return make_token(TOKEN_LEFT_BRACE);
		case '}': return make_token(TOKEN_RIGHT_BRACE);
		case ';': return make_token(TOKEN_SEMICOLON);
		case ':': return make_token(TOKEN_COLON);
		case ',': return make_token(TOKEN_COMMA);
		case '.': return make_token(TOKEN_DOT);
		case '-': return make_token(TOKEN_MINUS);
		case '+': return make_token(TOKEN_PLUS);
		case '*': return make_token(TOKEN_STAR);
		case '!': return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
		case '=': return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
		case '<': return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
		case '>': return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
		case '/':
			if (peek_next() == '/') {
				while (peek() != '\n' && !is_at_end()) {
					advance();
				}
			} else {
				return make_token(TOKEN_SLASH);
			}
			break;
		case '"':
			while (peek() != '"' && !is_at_end()) {
				if (peek() == '\n') line++;
				advance();
			}

			if (is_at_end()) {
				return make_error_token("Unterminated string.");
			}

			advance();
			return make_token(TOKEN_STRING);
	}

	return make_error_token("Unexpected character.");
}