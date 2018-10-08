#include <stdlib.h>
#include <stdio.h>

#include "lit.h"
#include "lit_parser.h"
#include "lit_debug.h"
#include "lit_memory.h"

static LitExpression* parse_expression(LitLexer* lexer);

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

static void synchronize(LitLexer* lexer) {
	advance(lexer);

	while (!is_at_end(lexer)) {
		switch (lexer->current.type) {
			case TOKEN_CLASS:
			case TOKEN_FUN:
			case TOKEN_VAR:
			case TOKEN_FOR:
			case TOKEN_IF:
			case TOKEN_WHILE:
			case TOKEN_SWITCH:
			case TOKEN_RETURN:
				return;
		}
	}

	advance(lexer);
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
	synchronize(lexer);
}

static LitToken consume(LitLexer* lexer, LitTokenType type, const char* message) {
	if (!match(lexer, type)) {
		error(lexer, &lexer->current, message);
		return lexer->current;
	}

	return lexer->previous;
}

static LitExpression* parse_primary(LitLexer* lexer) {
	if (match(lexer, TOKEN_TRUE)) {
		return (LitExpression*) lit_make_literal_expression(lexer->vm, TRUE_VALUE);
	}

	if (match(lexer, TOKEN_FALSE)) {
		return (LitExpression*) lit_make_literal_expression(lexer->vm, FALSE_VALUE);
	}

	if (match(lexer, TOKEN_NIL)) {
		return (LitExpression*) lit_make_literal_expression(lexer->vm, NIL_VALUE);
	}

	if (match(lexer, TOKEN_NUMBER)) {
		return (LitExpression*) lit_make_literal_expression(lexer->vm, MAKE_NUMBER_VALUE(strtod(lexer->previous.start, NULL)));
	}

	if (match(lexer, TOKEN_STRING)) {
		return (LitExpression*) lit_make_literal_expression(lexer->vm, MAKE_OBJECT_VALUE(lit_copy_string(lexer->vm, lexer->previous.start + 1, lexer->previous.length - 2)));
	}

	if (match(lexer, TOKEN_LEFT_PAREN)) {
		LitExpression* expression = parse_expression(lexer);
		consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
		return (LitExpression*) lit_make_grouping_expression(lexer->vm, expression);
	}
}

static LitExpression* parse_unary(LitLexer* lexer) {
	if (match(lexer, TOKEN_BANG) || match(lexer, TOKEN_MINUS)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression* right = parse_unary(lexer);
		return (LitExpression*) lit_make_unary_expression(lexer->vm, right, operator);
	}

	return parse_primary(lexer);
}

static LitExpression* parse_multiplication(LitLexer* lexer) {
	LitExpression *expression = parse_unary(lexer);

	while (match(lexer, TOKEN_STAR) || match(lexer, TOKEN_SLASH)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression *right = parse_unary(lexer);

		expression = (LitExpression*) lit_make_binary_expression(lexer->vm, expression, right, operator);
	}

	return expression;
}

static LitExpression* parse_addition(LitLexer* lexer) {
	LitExpression *expression = parse_multiplication(lexer);

	while (match(lexer, TOKEN_PLUS) || match(lexer, TOKEN_MINUS)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression *right = parse_multiplication(lexer);

		expression = (LitExpression*) lit_make_binary_expression(lexer->vm, expression, right, operator);
	}

	return expression;
}

static LitExpression* parse_comprasion(LitLexer *lexer) {
	LitExpression *expression = parse_addition(lexer);

	while (match(lexer, TOKEN_LESS) || match(lexer, TOKEN_LESS_EQUAL) || match(lexer, TOKEN_GREATER) || match(lexer, TOKEN_GREATER_EQUAL)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression *right = parse_addition(lexer);

		expression = (LitExpression*) lit_make_binary_expression(lexer->vm, expression, right, operator);
	}

	return expression;
}

static LitExpression* parse_equality(LitLexer* lexer) {
	LitExpression *expression = parse_comprasion(lexer);

	while (match(lexer, TOKEN_BANG_EQUAL) || match(lexer, TOKEN_EQUAL_EQUAL)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression* right = parse_comprasion(lexer);

		expression = (LitExpression*) lit_make_binary_expression(lexer->vm, expression, right, operator);
	}

	return expression;
}

static LitExpression* parse_expression(LitLexer* lexer) {
	return parse_equality(lexer);
}

static LitStatement* parse_expression_statement(LitLexer* lexer) {
	return (LitStatement*) lit_make_expression_statement(lexer->vm, parse_expression(lexer));
}

static LitStatement* parse_var_declaration(LitLexer* lexer) {
	LitToken name = consume(lexer, TOKEN_IDENTIFIER, "Expected variable name");
	LitExpression* init = NULL;

	if (match(lexer, TOKEN_EQUAL)) {
		init = parse_expression(lexer);
	}

	LitToken* token = (LitToken*) reallocate(lexer->vm, NULL, 0, sizeof(LitToken));

	token->start = name.start;
	token->length = name.length;
	token->line = name.line;
	token->type = name.type;

	return (LitStatement*) lit_make_var_statement(lexer->vm, token, init);
}

static LitStatement* parse_declaration(LitLexer* lexer) {
	if (match(lexer, TOKEN_VAR)) {
		return parse_var_declaration(lexer);
	}

	return parse_expression_statement(lexer);
}

LitStatements* lit_parse(LitVm* vm, LitStatements* statements) {
	LitLexer lexer;

	lit_init_lexer(&lexer, vm->code);
	lexer.vm = vm;
	advance(&lexer);

	while (!is_at_end(&lexer)) {
		lit_statements_write(vm, statements, parse_declaration(&lexer));
	}

	lit_free_lexer(&lexer);

	return statements;
}