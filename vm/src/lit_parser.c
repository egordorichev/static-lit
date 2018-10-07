#include <stdlib.h>
#include <stdio.h>

#include "lit.h"
#include "lit_parser.h"
#include "lit_debug.h"

static LitExpression *parse_expression(LitLexer* lexer);

static LitToken advance(LitLexer* lexer) {
	lexer->previous = lexer->current;
	lexer->current = lit_lexer_next_token(lexer);

	return lexer->current;
}

static inline bool is_at_end(LitLexer* lexer) {
	return lexer->current.type == TOKEN_EOF;
}

static bool match(LitLexer* lexer, LitTokenType type) {
	if (lexer->current.type == type) {
		advance(lexer);
		return true;
	}

	return false;
}

static void error(LitLexer* lexer, LitToken *token, const char* message) {
	if (lexer->panic_mode) {
		return;
	}

	lexer->panic_mode = true;
	lexer->had_error = true;

	printf("[line %d] Error", token->line);

	if (token->type == TOKEN_EOF) {
		printf(" at end");
	} else if (token->type != TOKEN_ERROR) {
		printf(" at '%.*s'", token->length, token->start);
	}

	printf(": %s\n", message);
}

static void consume(LitLexer* lexer, LitTokenType type, const char* message) {
	if (!match(lexer, type)) {
		error(lexer, &lexer->current, message);
	}
}

static LitExpression *parse_primary(LitLexer* lexer) {
	if (match(lexer, TOKEN_TRUE)) {
		return lit_make_literal_expression(lexer->vm, TRUE_VALUE);
	}

	if (match(lexer, TOKEN_FALSE)) {
		return lit_make_literal_expression(lexer->vm, FALSE_VALUE);
	}

	if (match(lexer, TOKEN_NIL)) {
		return lit_make_literal_expression(lexer->vm, NIL_VALUE);
	}

	if (match(lexer, TOKEN_NUMBER)) {
		return lit_make_literal_expression(lexer->vm, MAKE_NUMBER_VALUE(strtod(lexer->previous.start, NULL)));
	}

	if (match(lexer, TOKEN_STRING)) {
		return lit_make_literal_expression(lexer->vm, MAKE_OBJECT_VALUE(lit_copy_string(lexer->vm, lexer->previous.start + 1, lexer->previous.length - 2)));
	}

	if (match(lexer, TOKEN_LEFT_PAREN)) {
		LitExpression* expression = parse_expression(lexer);
		consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
		return lit_make_grouping_expression(lexer->vm, expression);
	}
}

static LitExpression *parse_unary(LitLexer* lexer) {
	if (match(lexer, TOKEN_BANG) || match(lexer, TOKEN_MINUS)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression* right = parse_unary(lexer);
		return lit_make_unary_expression(lexer->vm, right, operator);
	}

	return parse_primary(lexer);
}

static LitExpression *parse_multiplication(LitLexer* lexer) {
	LitExpression *expression = parse_unary(lexer);

	while (match(lexer, TOKEN_STAR) || match(lexer, TOKEN_SLASH)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression *right = parse_unary(lexer);

		expression = lit_make_binary_expression(lexer->vm, expression, right, operator);
	}

	return expression;
}

static LitExpression *parse_addition(LitLexer* lexer) {
	LitExpression *expression = parse_multiplication(lexer);

	while (match(lexer, TOKEN_PLUS) || match(lexer, TOKEN_MINUS)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression *right = parse_multiplication(lexer);

		expression = lit_make_binary_expression(lexer->vm, expression, right, operator);
	}

	return expression;
}

static LitExpression *parse_comprasion(LitLexer *lexer) {
	LitExpression *expression = parse_addition(lexer);

	while (match(lexer, TOKEN_LESS) || match(lexer, TOKEN_LESS_EQUAL) || match(lexer, TOKEN_GREATER) || match(lexer, TOKEN_GREATER_EQUAL)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression *right = parse_addition(lexer);

		expression = lit_make_binary_expression(lexer->vm, expression, right, operator);
	}

	return expression;
}

static LitExpression *parse_equality(LitLexer* lexer) {
	LitExpression *expression = parse_comprasion(lexer);

	while (match(lexer, TOKEN_BANG_EQUAL) || match(lexer, TOKEN_EQUAL_EQUAL)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression* right = parse_comprasion(lexer);

		expression = lit_make_binary_expression(lexer->vm, expression, right, operator);
	}

	return expression;
}

static LitExpression *parse_expression(LitLexer* lexer) {
	return parse_equality(lexer);
}

LitExpression *lit_parse(LitVm* vm) {
	LitLexer lexer;

	lit_init_lexer(&lexer, vm->code);
	lexer.vm = vm;

	advance(&lexer);
	LitExpression* expression = parse_equality(&lexer);

	lit_free_lexer(&lexer);

	return expression;
}