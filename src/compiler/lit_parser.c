#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include <lit.h>
#include <lit_debug.h>
#include <compiler/lit_parser.h>
#include <vm/lit_memory.h>
#include <compiler/lit_lexer.h>

static LitExpression* parse_expression(LitLexer* lexer);
static LitStatement* parse_statement(LitLexer* lexer);
static LitStatement* parse_declaration(LitLexer* lexer);
static LitStatement* parse_var_declaration(LitLexer* lexer, bool final);

static const char* copy_string(LitLexer* lexer, LitToken* name) {
	char* str = (char*) reallocate(lexer->compiler, NULL, 0, (size_t) name->length + (size_t) 1);
	strncpy(str, name->start, (size_t) name->length);
	str[name->length] = '\0';

	return str;
}

static const char* copy_string_native(LitLexer* lexer, const char* name, uint64_t length) {
	char* str = (char*) reallocate(lexer->compiler, NULL, 0, (size_t) length + (size_t) 1);
	strncpy(str, name, (size_t) length);
	str[length] = '\0';

	return str;
}

static void error(LitLexer* lexer, LitToken *token, const char* message);

static LitToken advance(LitLexer* lexer) {
	lexer->previous = lexer->current;

	do {
		lexer->current = lit_lexer_next_token(lexer);

		if (lexer->current.type == TOKEN_ERROR) {
			error(lexer, &lexer->current, lexer->current.start);
		}
	} while (lexer->current.type == TOKEN_ERROR);

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
			case TOKEN_VAR:
			case TOKEN_FOR:
			case TOKEN_IF:
			case TOKEN_WHILE:
			case TOKEN_SWITCH:
			case TOKEN_RETURN:
				return;
			default: continue;
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

	fflush(stdout);
	fprintf(stderr, "[line %lu] Error", token->line);

	if (token->type == TOKEN_EOF) {
		fprintf(stderr, " at end");
	} else if (token->type != TOKEN_ERROR) {
		fprintf(stderr, " at '%.*s'", (int) token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);
	fflush(stderr);

	synchronize(lexer);
}

static LitToken consume(LitLexer* lexer, LitTokenType type, const char* message) {
	if (!match(lexer, type)) {
		error(lexer, &lexer->current, message);
		return lexer->current;
	}

	return lexer->previous;
}

static LitExpression* parse_if_expression(LitLexer* lexer) {
	uint64_t line = lexer->line;
	LitExpression* condition = parse_expression(lexer);
	LitExpression* if_branch = parse_expression(lexer);

	LitExpression* else_branch = NULL;
	LitExpressions* else_if_branches = NULL;
	LitExpressions* else_if_conditions = NULL;

	while (match(lexer, TOKEN_ELSE)) {
		if (match(lexer, TOKEN_IF)) {
			if (else_branch != NULL) {
				error(lexer, &lexer->current, "Else branch is already defined for this if");
				return NULL;
			}

			if (else_if_branches == NULL) {
				else_if_conditions = (LitExpressions*) reallocate(lexer->compiler, NULL, 0, sizeof(LitExpressions));
				lit_init_expressions(else_if_conditions);

				else_if_branches = (LitExpressions*) reallocate(lexer->compiler, NULL, 0, sizeof(LitExpressions));
				lit_init_expressions(else_if_branches);
			}

			lit_expressions_write(MM(lexer->compiler), else_if_conditions, parse_expression(lexer));
			lit_expressions_write(MM(lexer->compiler), else_if_conditions, parse_expression(lexer));
		} else {
			else_branch = parse_expression(lexer);
		}
	}

	if (else_branch == NULL) {
		error(lexer, &lexer->previous, "If expression must have else branch");
		return NULL;
	}

	return (LitExpression*) lit_make_if_expression(lexer->compiler, line, condition, if_branch, else_branch, else_if_branches, else_if_conditions);
}

static LitExpression* parse_lambda(LitLexer* lexer);

static LitExpression* parse_primary(LitLexer* lexer) {
	if (match(lexer, TOKEN_THIS)) {
		return (LitExpression*) lit_make_this_expression(lexer->compiler, lexer->last_line);
	}

	if (match(lexer, TOKEN_SUPER)) {
		uint64_t line = lexer->last_line;

		consume(lexer, TOKEN_DOT, "Expected '.' after super");
		LitToken token = consume(lexer, TOKEN_IDENTIFIER, "Expected method name after dot");

		return (LitExpression*) lit_make_super_expression(lexer->compiler, line, copy_string(lexer, &token));
	}

	if (match(lexer, TOKEN_IDENTIFIER)) {
		return (LitExpression*) lit_make_var_expression(lexer->compiler, lexer->last_line, copy_string(lexer, &lexer->previous));
	}

	if (match(lexer, TOKEN_TRUE)) {
		return (LitExpression*) lit_make_literal_expression(lexer->compiler, lexer->last_line, TRUE_VALUE);
	}

	if (match(lexer, TOKEN_FALSE)) {
		return (LitExpression*) lit_make_literal_expression(lexer->compiler, lexer->last_line, FALSE_VALUE);
	}

	if (match(lexer, TOKEN_NIL)) {
		return (LitExpression*) lit_make_literal_expression(lexer->compiler, lexer->last_line, NIL_VALUE);
	}

	if (match(lexer, TOKEN_NUMBER)) {
		return (LitExpression*) lit_make_literal_expression(lexer->compiler, lexer->last_line, MAKE_NUMBER_VALUE(strtod(lexer->previous.start, NULL)));
	}

	if (match(lexer, TOKEN_STRING)) {
		return (LitExpression*) lit_make_literal_expression(lexer->compiler, lexer->last_line,
			MAKE_OBJECT_VALUE(lit_copy_string(MM(lexer->compiler), lexer->previous.start + 1, lexer->previous.length - 2)));
	}

	if (match(lexer, TOKEN_CHAR)) {
		return (LitExpression*) lit_make_literal_expression(lexer->compiler, lexer->last_line, MAKE_CHAR_VALUE((unsigned char) lexer->previous.start[1]));
	}

	if (match(lexer, TOKEN_LEFT_PAREN)) {
		uint64_t line = lexer->last_line;

		LitExpression* expression = parse_expression(lexer);
		consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after expression");

		return (LitExpression*) lit_make_grouping_expression(lexer->compiler, line, expression);
	}

	/*
	if (match(lexer, TOKEN_EQUAL)) {
		consume(lexer, TOKEN_GREATER, "Expected '>' in lambda declaration");
		return parse_lambda(lexer);
	}*/ // fixme: lambdas

	error(lexer, &lexer->current, "Unexpected token");
	return NULL;
}

static LitExpression* finish_call(LitLexer* lexer, LitExpression* callee) {
	uint64_t line = lexer->last_line;
	LitExpressions* args = (LitExpressions*) reallocate(lexer->compiler, NULL, 0, sizeof(LitExpressions));
	lit_init_expressions(args);

	if (lexer->current.type != TOKEN_RIGHT_PAREN) {
		do {
			lit_expressions_write(MM(lexer->compiler), args, parse_expression(lexer));
		} while (match(lexer, TOKEN_COMMA));
	}

	consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after arguments");
	return (LitExpression*) lit_make_call_expression(lexer->compiler, line, callee, args);
}

static LitExpression* parse_equality(LitLexer* lexer);

static LitExpression* parse_call(LitLexer* lexer) {
	LitExpression* expression = parse_primary(lexer);

	while (true) {
		if (match(lexer, TOKEN_LEFT_PAREN)) {
			expression = finish_call(lexer, expression);
		} else if (match(lexer, TOKEN_DOT)) {
			uint64_t line = lexer->last_line;
			LitToken token = consume(lexer, TOKEN_IDENTIFIER, "Expected property name after '.'");

			if (match(lexer, TOKEN_EQUAL)) {
				expression = (LitExpression*) lit_make_set_expression(lexer->compiler, line, expression, parse_equality(lexer), copy_string(lexer, &token));
			} else {
				expression = (LitExpression*) lit_make_get_expression(lexer->compiler, line, expression, copy_string(lexer, &token));
			}
		} else {
			break;
		}
	}

	return expression;
}

static LitExpression* parse_compound_power(LitLexer* lexer) {
	LitExpression* expression = parse_call(lexer);

	if (match(lexer, TOKEN_CARET_EQUAL) || match(lexer, TOKEN_CELL_EQUAL)) {
		/*
		 * Desugar this:
		 * a ^= 10
		 * into this:
		 * a = a ^ 10
		 */

		uint64_t line = lexer->last_line;
		LitTokenType type = lexer->previous.type;
		LitExpression* operator = NULL;

		if (type == TOKEN_CARET_EQUAL) {
			LitBinaryExpression* bin = lit_make_binary_expression(lexer->compiler, line, expression, parse_call(lexer), TOKEN_CARET);
			bin->ignore_left = true; // Because its already freed from another expression
			operator = (LitExpression *) bin;
		} else if (type == TOKEN_CELL_EQUAL) {
			LitBinaryExpression* bin = lit_make_binary_expression(lexer->compiler, line, expression, parse_call(lexer), TOKEN_CELL);
			bin->ignore_left = true; // Because its already freed from another expression
			operator = (LitExpression *) bin;
		}

		return (LitExpression *) lit_make_assign_expression(lexer->compiler, line, expression, operator);
	}

	return expression;
}

static LitExpression* parse_compound_multiplication(LitLexer* lexer) {
	LitExpression* expression = parse_compound_power(lexer);

	if (match(lexer, TOKEN_STAR_EQUAL) || match(lexer, TOKEN_SLASH_EQUAL) || match(lexer, TOKEN_PERCENT_EQUAL)) {
		/*
		 * Desugar this:
		 * a *= 10
		 * into this:
		 * a = a * 10
		 */

		uint64_t line = lexer->last_line;
		LitTokenType type = lexer->previous.type;
		LitExpression* operator = NULL;

		if (type == TOKEN_STAR_EQUAL) {
			LitBinaryExpression* bin = lit_make_binary_expression(lexer->compiler, line, expression, parse_compound_power(lexer), TOKEN_STAR);
			bin->ignore_left = true; // Because its already freed from another expression
			operator = (LitExpression *) bin;
		} else if (type == TOKEN_SLASH_EQUAL) {
			LitBinaryExpression* bin = lit_make_binary_expression(lexer->compiler, line, expression, parse_compound_power(lexer), TOKEN_SLASH);
			bin->ignore_left = true; // Because its already freed from another expression
			operator = (LitExpression *) bin;
		} else {
			LitBinaryExpression* bin = lit_make_binary_expression(lexer->compiler, line, expression, parse_compound_power(lexer), TOKEN_PERCENT);
			bin->ignore_left = true; // Because its already freed from another expression
			operator = (LitExpression *) bin;
		}

		return (LitExpression *) lit_make_assign_expression(lexer->compiler, line, expression, operator);
	}

	return expression;
}

static LitExpression* parse_compound_addition(LitLexer* lexer) {
	LitExpression* expression = parse_compound_multiplication(lexer);

	if (match(lexer, TOKEN_PLUS_EQUAL) || match(lexer, TOKEN_MINUS_EQUAL) || match(lexer, TOKEN_PLUS_PLUS) || match(lexer, TOKEN_MINUS_MINUS)) {
		/*
		 * Desugar this:
		 * a += 10 or a++
		 * into this:
		 * a = a + 10 or a = a + 1
		 */

		uint64_t line = lexer->last_line;
		LitTokenType type = lexer->previous.type;
		LitExpression* operator = NULL;

		if (type == TOKEN_PLUS_EQUAL) {
			LitBinaryExpression* bin = lit_make_binary_expression(lexer->compiler, line, expression, parse_compound_multiplication(lexer), TOKEN_PLUS);
			bin->ignore_left = true; // Because its already freed from another expression
			operator = (LitExpression *) bin;
		} else if (type == TOKEN_MINUS_EQUAL) {
			LitBinaryExpression* bin = lit_make_binary_expression(lexer->compiler, line, expression, parse_compound_multiplication(lexer), TOKEN_MINUS);
			bin->ignore_left = true; // Because its already freed from another expression
			operator = (LitExpression *) bin;
		} else if (type == TOKEN_PLUS_PLUS) {
			LitBinaryExpression* bin = lit_make_binary_expression(lexer->compiler, line, expression, (LitExpression *) lit_make_literal_expression(lexer->compiler, line, MAKE_NUMBER_VALUE(1)), TOKEN_PLUS);
			bin->ignore_left = true; // Because its already freed from another expression
			operator = (LitExpression *) bin;
		} else {
			LitBinaryExpression* bin = lit_make_binary_expression(lexer->compiler, line, expression, (LitExpression *) lit_make_literal_expression(lexer->compiler, line, MAKE_NUMBER_VALUE(1)), TOKEN_MINUS);
			bin->ignore_left = true; // Because its already freed from another expression
			operator = (LitExpression *) bin;
		}

		return (LitExpression *) lit_make_assign_expression(lexer->compiler, line, expression, operator);
	}

	return expression;
}

static LitExpression* parse_is(LitLexer* lexer) {
	LitExpression *expression = parse_compound_addition(lexer);

	if (match(lexer, TOKEN_IS)) {
		uint64_t line = lexer->last_line;
		LitTokenType operator = lexer->previous.type;
		expression = (LitExpression*) lit_make_binary_expression(lexer->compiler, line, expression, parse_compound_addition(lexer), operator);
	}

	return expression;
}

static LitExpression* parse_unary(LitLexer* lexer) {
	if (match(lexer, TOKEN_BANG) || match(lexer, TOKEN_MINUS) || match(lexer, TOKEN_CELL)) {
		uint64_t line = lexer->last_line;
		LitTokenType operator = lexer->previous.type;
		return (LitExpression*) lit_make_unary_expression(lexer->compiler, line, parse_unary(lexer), operator);
	}

	return parse_is(lexer);
}

static LitExpression* parse_power(LitLexer* lexer) {
	LitExpression* expression = parse_unary(lexer);

	while (match(lexer, TOKEN_CARET) || match(lexer, TOKEN_CELL)) {
		uint64_t line = lexer->last_line;
		LitTokenType operator = lexer->previous.type;
		expression = (LitExpression*) lit_make_binary_expression(lexer->compiler, line, expression, parse_unary(lexer), operator);
	}

	return expression;
}

static LitExpression* parse_multiplication(LitLexer* lexer) {
	LitExpression* expression = parse_power(lexer);

	while (match(lexer, TOKEN_STAR) || match(lexer, TOKEN_SLASH) || match(lexer, TOKEN_PERCENT)) {
		uint64_t line = lexer->last_line;
		LitTokenType operator = lexer->previous.type;
		expression = (LitExpression*) lit_make_binary_expression(lexer->compiler, line, expression, parse_power(lexer), operator);
	}

	return expression;
}

static LitExpression* parse_addition(LitLexer* lexer) {
	LitExpression* expression = parse_multiplication(lexer);

	while (match(lexer, TOKEN_PLUS) || match(lexer, TOKEN_MINUS)) {
		uint64_t line = lexer->last_line;
		LitTokenType operator = lexer->previous.type;
		expression = (LitExpression*) lit_make_binary_expression(lexer->compiler, line, expression, parse_multiplication(lexer), operator);
	}

	return expression;
}

static LitExpression* parse_comprasion(LitLexer *lexer) {
	LitExpression* expression = parse_addition(lexer);

	while (match(lexer, TOKEN_LESS) || match(lexer, TOKEN_LESS_EQUAL) || match(lexer, TOKEN_GREATER) || match(lexer, TOKEN_GREATER_EQUAL)) {
		uint64_t line = lexer->last_line;
		LitTokenType operator = lexer->previous.type;
		expression = (LitExpression*) lit_make_binary_expression(lexer->compiler, line, expression, parse_addition(lexer), operator);
	}

	return expression;
}

static LitExpression* parse_equality(LitLexer* lexer) {
	LitExpression* expression = parse_comprasion(lexer);

	while (match(lexer, TOKEN_BANG_EQUAL) || match(lexer, TOKEN_EQUAL_EQUAL)) {
		uint64_t line = lexer->last_line;
		LitTokenType operator = lexer->previous.type;
		expression = (LitExpression*) lit_make_binary_expression(lexer->compiler, line, expression, parse_comprasion(lexer), operator);
	}

	return expression;
}

static LitExpression* parse_and(LitLexer* lexer) {
	LitExpression* expression = parse_equality(lexer);

	while (match(lexer, TOKEN_AND)) {
		uint64_t line = lexer->last_line;
		LitTokenType operator = lexer->previous.type;
		expression = (LitExpression*) lit_make_logical_expression(lexer->compiler, line, operator, expression, parse_equality(lexer));
	}

	return expression;
}

static LitExpression* parse_or(LitLexer* lexer) {
	LitExpression* expression = parse_and(lexer);

	while (match(lexer, TOKEN_OR)) {
		uint64_t line = lexer->last_line;
		LitTokenType operator = lexer->previous.type;
		expression = (LitExpression*) lit_make_logical_expression(lexer->compiler, line, operator, expression, parse_and(lexer));
	}

	return expression;
}

static LitExpression* parse_short_if(LitLexer* lexer) {
	uint64_t line = lexer->line;
	LitExpression* expression = parse_or(lexer);

	if (match(lexer, TOKEN_QUESTION)) {
		LitExpression* if_branch = parse_expression(lexer);
		consume(lexer, TOKEN_COLON, "Expected ':'");
		LitExpression* else_branch = parse_expression(lexer);

		return (LitExpression*) lit_make_if_expression(lexer->compiler, line, expression, if_branch, else_branch, NULL, NULL);
	}

	return expression;
}

static LitExpression* parse_if_expr(LitLexer* lexer) {
	if (match(lexer, TOKEN_IF)) {
		return parse_if_expression(lexer);
	}

	return parse_short_if(lexer);
}

static LitExpression* parse_assigment(LitLexer* lexer) {
	LitExpression* expression = parse_if_expr(lexer);

	if (match(lexer, TOKEN_EQUAL)) {
		uint64_t line = lexer->last_line;
		LitToken equal = lexer->previous;
		LitExpression* value = parse_assigment(lexer);

		if (expression->type == VAR_EXPRESSION) {
			return (LitExpression*) lit_make_assign_expression(lexer->compiler, line, expression, value);
		}

		error(lexer, &equal, "Invalid assignment target");
		return NULL;
	}

	return expression;
}

static LitExpression* parse_expression(LitLexer* lexer) {
	return parse_assigment(lexer);
}

static LitStatement* parse_expression_statement(LitLexer* lexer) {
	uint64_t line = lexer->last_line;
	return (LitStatement*) lit_make_expression_statement(lexer->compiler, line, parse_expression(lexer));
}

static LitStatement* parse_if_statement(LitLexer* lexer) {
	uint64_t line = lexer->last_line;
	LitExpression* condition = parse_expression(lexer);

	LitStatement* if_branch = parse_statement(lexer);
	LitStatement* else_branch = NULL;
	LitStatements* else_if_branches = NULL;
	LitExpressions* else_if_conditions = NULL;

	while (match(lexer, TOKEN_ELSE)) {
		if (match(lexer, TOKEN_IF)) {
			if (else_branch != NULL) {
				error(lexer, &lexer->current, "Else branch is already defined for this if");
				return NULL;
			}

			if (else_if_branches == NULL) {
				else_if_conditions = (LitExpressions*) reallocate(lexer->compiler, NULL, 0, sizeof(LitExpressions));
				lit_init_expressions(else_if_conditions);

				else_if_branches = (LitStatements*) reallocate(lexer->compiler, NULL, 0, sizeof(LitStatements));
				lit_init_statements(else_if_branches);
			}

			lit_expressions_write(MM(lexer->compiler), else_if_conditions, parse_expression(lexer));
			lit_statements_write(MM(lexer->compiler), else_if_branches, parse_statement(lexer));
		} else {
			else_branch = parse_statement(lexer);
		}
	}

	return (LitStatement*) lit_make_if_statement(lexer->compiler, line, condition, if_branch, else_branch, else_if_branches, else_if_conditions);
}

static LitStatement* parse_while(LitLexer* lexer) {
	uint64_t line = lexer->last_line;
	LitExpression* condition = parse_expression(lexer);
	return (LitStatement*) lit_make_while_statement(lexer->compiler, line, condition, parse_statement(lexer));
}

static LitStatement* parse_for(LitLexer* lexer) {
	uint64_t line = lexer->line;
	bool had_paren = match(lexer, TOKEN_LEFT_PAREN);
	LitStatement* init = NULL;

	if (match(lexer, TOKEN_VAR)) {
		init = parse_var_declaration(lexer, false);
		consume(lexer, TOKEN_SEMICOLON, "Expected ; after for init");
	} else if (!match(lexer, TOKEN_SEMICOLON)) {
		init = parse_expression_statement(lexer);
		consume(lexer, TOKEN_SEMICOLON, "Expected ; after for init");
	}

	LitExpression* condition = NULL;

	if (lexer->current.type != TOKEN_SEMICOLON) {
		condition = parse_expression(lexer);
	}

	consume(lexer, TOKEN_SEMICOLON, "Expected ; after for condition");
	LitExpression* increment = NULL;

	if (lexer->current.type != TOKEN_SEMICOLON) {
		increment = parse_expression(lexer);
	}

	if (had_paren) {
		consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after for");
	}

	LitStatement* body = parse_statement(lexer);

	if (increment != NULL) {
		LitStatements* statements = (LitStatements*) reallocate(lexer->compiler, NULL, 0, sizeof(LitStatements));
		lit_init_statements(statements);

		lit_statements_write(MM(lexer->compiler), statements, body);
		lit_statements_write(MM(lexer->compiler), statements, (LitStatement*) lit_make_expression_statement(lexer->compiler, line, increment));

		body = (LitStatement*) lit_make_block_statement(lexer->compiler, line, statements);
	}

	if (condition == NULL) {
		condition = (LitExpression*) lit_make_literal_expression(lexer->compiler, line, TRUE_VALUE);
	}

	body = (LitStatement*) lit_make_while_statement(lexer->compiler, line, condition, body);

	if (init != NULL) {
		LitStatements* statements = (LitStatements*) reallocate(lexer->compiler, NULL, 0, sizeof(LitStatements));
		lit_init_statements(statements);

		lit_statements_write(MM(lexer->compiler), statements, init);
		lit_statements_write(MM(lexer->compiler), statements, body);

		body = (LitStatement*) lit_make_block_statement(lexer->compiler, line, statements);
	}

	return body;
}

static LitStatement* parse_block_statement(LitLexer* lexer) {
	LitStatements* statements = NULL;
	uint64_t line = lexer->last_line;

	while (!match(lexer, TOKEN_RIGHT_BRACE)) {
		if (statements == NULL) {
			statements = (LitStatements*) reallocate(lexer->compiler, NULL, 0, sizeof(LitStatements));
			lit_init_statements(statements);
		}

		if (lexer->current.type == TOKEN_EOF) {
			error(lexer, &lexer->current, "Expected '}' to close the block");
			return NULL;
		}

		lit_statements_write(MM(lexer->compiler), statements, parse_declaration(lexer));
	}

	return (LitStatement*) lit_make_block_statement(lexer->compiler, line, statements);
}

static char* parse_argument_type(LitLexer* lexer) {
	consume(lexer, TOKEN_IDENTIFIER, "Expected argument type");
	char* start = (char*) lexer->previous.start;

	if (!match(lexer, TOKEN_LESS)) {
		return (char*) copy_string_native(lexer, start, lexer->previous.length);
	}

	while (!match(lexer, TOKEN_GREATER)) {
		consume(lexer, TOKEN_IDENTIFIER, "Expected template type name");

		if (lexer->current.type != TOKEN_GREATER) {
			consume(lexer, TOKEN_COMMA, "Expected ',' after template type name");
		}
	}

	return (char*) copy_string_native(lexer, start, (uint64_t) (lexer->current.start - start - 1));
}

static LitExpression* parse_lambda(LitLexer* lexer) {
	uint64_t line = lexer->last_line;
	consume(lexer, TOKEN_LEFT_PAREN, "Expected '(' after 'fun'");
	LitParameters* parameters = NULL;

	if (lexer->current.type != TOKEN_RIGHT_PAREN) {
		parameters = (LitParameters*) reallocate(lexer->compiler, NULL, 0, sizeof(LitParameters));
		lit_init_parameters(parameters);

		do {
			char* type = parse_argument_type(lexer);
			LitToken name = consume(lexer, TOKEN_IDENTIFIER, "Expected argument name");

			lit_parameters_write(MM(lexer->compiler), parameters, (LitParameter) {copy_string_native(lexer, name.start, name.length), type});
		} while (match(lexer, TOKEN_COMMA));
	}

	consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");

	LitParameter return_type = (LitParameter) {NULL, "void"};

	if (match(lexer, TOKEN_GREATER)) {
		LitToken type = consume(lexer, TOKEN_IDENTIFIER, "Expected return type");

		return_type.type = copy_string(lexer, &type);
	}

	consume(lexer, TOKEN_LEFT_BRACE, "Expected '{' before lambda body");
	return (LitExpression*) lit_make_lambda_expression(lexer->compiler, line, parameters, parse_block_statement(lexer), (LitParameter) {NULL, return_type.type});
}

static LitStatement* parse_function_statement(LitLexer* lexer, const char* return_type, char* name) {
	uint64_t line = lexer->last_line;
	consume(lexer, TOKEN_LEFT_PAREN, "Expected '(' after function name");

	LitParameters* parameters = NULL;

	if (lexer->current.type != TOKEN_RIGHT_PAREN) {
		parameters = (LitParameters*) reallocate(lexer->compiler, NULL, 0, sizeof(LitParameters));
		lit_init_parameters(parameters);

		do {
			char* type = parse_argument_type(lexer);
			LitToken argName = consume(lexer, TOKEN_IDENTIFIER, "Expected argument name");

			lit_parameters_write(MM(lexer->compiler), parameters, (LitParameter) {copy_string_native(lexer, argName.start, argName.length), type});
		} while (match(lexer, TOKEN_COMMA));
	}

	consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");

	LitStatement* body;

	if (match(lexer, TOKEN_ARROW)) {
		uint64_t bodyLine = lexer->last_line;
		body = (LitStatement *) lit_make_return_statement(lexer->compiler, bodyLine, parse_expression(lexer));
	} else {
		consume(lexer, TOKEN_LEFT_BRACE, "Expected '{' before function body");
		body = parse_block_statement(lexer);
	}

	return (LitStatement*) lit_make_function_statement(lexer->compiler, line, name, parameters, body, (LitParameter) {NULL, return_type});
}

static LitStatement* parse_method_statement(LitLexer* lexer, bool final, bool abstract, bool override, bool is_static, LitAccessType access, const char* return_type, char *name) {
	uint64_t line = lexer->last_line;

	if (final) {
		error(lexer, &lexer->previous, "Can't declare a final method");
		return NULL;
	}

	if (is_static && override) {
		error(lexer, &lexer->previous, "Can't override a static class");
		return NULL;
	}

	consume(lexer, TOKEN_LEFT_PAREN, "Expected '(' after method name");

	LitParameters* parameters = NULL;

	if (lexer->current.type != TOKEN_RIGHT_PAREN) {
		parameters = (LitParameters*) reallocate(lexer->compiler, NULL, 0, sizeof(LitParameters));
		lit_init_parameters(parameters);

		do {
			char* type = parse_argument_type(lexer);
			LitToken argName = consume(lexer, TOKEN_IDENTIFIER, "Expected argument name");

			lit_parameters_write(MM(lexer->compiler), parameters, (LitParameter) {copy_string_native(lexer, argName.start, argName.length), type});
		} while (match(lexer, TOKEN_COMMA));
	}

	consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");

	LitStatement* body = NULL;

	if (match(lexer, TOKEN_LEFT_BRACE)) {
		if (abstract) {
			error(lexer, &lexer->previous, "Abstract method can't have body");
			return NULL;
		} else {
			body = parse_block_statement(lexer);
		}
	} else if (match(lexer, TOKEN_ARROW)) {
		uint64_t bodyLine = lexer->last_line;

		if (abstract) {
			error(lexer, &lexer->previous, "Abstract method can't have body");
			return NULL;
		} else {
			body = (LitStatement *) lit_make_return_statement(lexer->compiler, bodyLine, parse_expression(lexer));
		}
	} else if (!abstract) {
		error(lexer, &lexer->previous, "Only abstract methods can have no body");
		return NULL;
	}

	return (LitStatement*) lit_make_method_statement(lexer->compiler, line, name, parameters, body, (LitParameter) {NULL, return_type}, override, is_static, abstract, access);
}

static LitStatement* parse_return_statement(LitLexer* lexer) {
	uint64_t line = lexer->last_line;
	return (LitStatement*) lit_make_return_statement(lexer->compiler, line, (lexer->current.type != TOKEN_RIGHT_BRACE && lexer->current.type != TOKEN_EOF) ? parse_expression(lexer) : NULL);
}

static LitStatement* parse_statement(LitLexer* lexer) {
	if (match(lexer, TOKEN_LEFT_BRACE)) {
		return parse_block_statement(lexer);
	}

	if (match(lexer, TOKEN_IF)) {
		return parse_if_statement(lexer);
	}

	if (match(lexer, TOKEN_RETURN)) {
		return parse_return_statement(lexer);
	}

	if (match(lexer, TOKEN_FOR)) {
		return parse_for(lexer);
	}

	if (match(lexer, TOKEN_WHILE)) {
		return parse_while(lexer);
	}

	if (match(lexer, TOKEN_BREAK)) {
		return (LitStatement *) lit_make_break_statement(lexer->compiler, lexer->last_line);
	}

	if (match(lexer, TOKEN_CONTINUE)) {
		return (LitStatement *) lit_make_continue_statement(lexer->compiler, lexer->last_line);
	}

	return parse_expression_statement(lexer);
}

static LitStatement* parse_var_declaration(LitLexer* lexer, bool final) {
	uint64_t line = lexer->last_line;
	LitToken name = consume(lexer, TOKEN_IDENTIFIER, "Expected variable name");
	LitExpression* init = NULL;

	if (match(lexer, TOKEN_EQUAL)) {
		init = parse_expression(lexer);
	}

	return (LitStatement*) lit_make_var_statement(lexer->compiler, line, copy_string(lexer, &name), init, NULL, final);
}

static LitStatement* parse_extended_var_declaration(LitLexer* lexer, LitToken* type, LitToken* name, bool final) {
	uint64_t line = lexer->last_line;
	char* name_str = (char *) copy_string(lexer, name);
	char* type_str = (char *) copy_string(lexer, type);

	advance(lexer);
	LitExpression* init = NULL;

	if (match(lexer, TOKEN_EQUAL)) {
		init = parse_expression(lexer);
	}

	return (LitStatement*) lit_make_var_statement(lexer->compiler, line, name_str, init, type_str, final);
}

static LitStatement* parse_field_declaration(LitLexer* lexer, bool final, bool abstract, bool override, bool is_static, LitAccessType access, char* type, char* name) {
	LitExpression* init = NULL;
	uint64_t line = lexer->last_line;

	if (abstract) {
		error(lexer, &lexer->previous, "Can't declare abstract field");
		return NULL;
	}

	if (override) {
		error(lexer, &lexer->previous, "Can't override a field");
		return NULL;
	}

	if (match(lexer, TOKEN_EQUAL)) {
		init = parse_expression(lexer);
	}

	if (final && init == NULL) {
		error(lexer, &lexer->previous, "Can't declare a final variable without value");
		return NULL;
	}

	if (type == NULL && init == NULL) {
		error(lexer, &lexer->previous, "Can't declare a field without type");
		return NULL;
	}

	return (LitStatement*) lit_make_field_statement(lexer->compiler, line, name, init, type, NULL, NULL, access, is_static, final);
}

static LitStatement* parse_class_declaration(LitLexer* lexer, bool abstract, bool is_static, bool final, bool had_class_token) {
	uint64_t line = lexer->last_line;

	if (!had_class_token) {
		consume(lexer, TOKEN_CLASS, "Expected 'class'");
	}

	LitToken name_token = consume(lexer, TOKEN_IDENTIFIER, "Expect class name");
	char* class_name = (char *) copy_string(lexer, &name_token);
	LitVarExpression* super = NULL;

	if (match(lexer, TOKEN_LESS)) {
		if (is_static) {
			error(lexer, &lexer->previous, "Static classes can't inherit other classes");
			return NULL;
		}

		consume(lexer, TOKEN_IDENTIFIER, "Expected superclass name");
		super = lit_make_var_expression(lexer->compiler, line, copy_string(lexer, &lexer->previous));
	}

	LitMethods* methods = NULL;
	LitStatements* fields = NULL;

	if (match(lexer, TOKEN_LEFT_BRACE)) {
		while (lexer->current.type != TOKEN_RIGHT_BRACE && !is_at_end(lexer)) {
			bool field_is_static = is_static;
			bool final = false;
			bool override = false;
			bool is_abstract = abstract;
			LitAccessType access = UNDEFINED_ACCESS;

			while (match(lexer, TOKEN_PUBLIC) || match(lexer, TOKEN_PROTECTED) || match(lexer, TOKEN_PRIVATE) || match(lexer, TOKEN_STATIC) || match(lexer, TOKEN_FINAL) || match(lexer, TOKEN_OVERRIDE) || match(lexer, TOKEN_ABSTRACT)) {
				switch(lexer->previous.type) {
					case TOKEN_FINAL: {
						if (final) {
							error(lexer, &lexer->previous, "Final modifier used twice or more per field");
							return NULL;
						} else {
							final = true;
						}

						break;
					}
					case TOKEN_STATIC: {
						if (field_is_static) {
							error(lexer, &lexer->previous, is_static ? "No need to use static field modifier in a static class" : "Static keyword used twice or more per field");
							return NULL;
						}

						field_is_static = true;
						break;
					}
					case TOKEN_PRIVATE: {
						if (access != UNDEFINED_ACCESS) {
							error(lexer, &lexer->previous, "Access modifier used twice or more per field");
							return NULL;
						}
	
						access = PRIVATE_ACCESS;
						break;
					}
					case TOKEN_PROTECTED: {
						if (access != UNDEFINED_ACCESS) {
							error(lexer, &lexer->previous, "Access modifier used twice or more per field");
							return NULL;
						}
	
						access = PROTECTED_ACCESS;
						break;
					}
					case TOKEN_PUBLIC: {
						if (access != UNDEFINED_ACCESS) {
							error(lexer, &lexer->previous, "Access modifier used twice or more per field");
							return NULL;
						}
	
						access = PUBLIC_ACCESS;
						break;
					}
					case TOKEN_OVERRIDE: {
						if (override) {
							error(lexer, &lexer->previous, "Override keyword used twice or more per field");
							return NULL;
						}

						override = true;
						break;
					}
					case TOKEN_ABSTRACT: {
						if (is_abstract) {
							error(lexer, &lexer->previous, "Abstract keyword used twice or more per field");
							return NULL;
						}

						is_abstract = true;
						break;
					}
					default: UNREACHABLE();
				}
			}

			if (match(lexer, TOKEN_VAR)) {
				if (access == UNDEFINED_ACCESS) {
					access = PROTECTED_ACCESS;
				}

				if (fields == NULL) {
					fields = (LitStatements*) reallocate(lexer->compiler, NULL, 0, sizeof(LitStatements));
					lit_init_statements(fields);
				}

				char* name = (char *) copy_string(lexer, &lexer->current);
				advance(lexer);
				lit_statements_write(MM(lexer->compiler), fields, parse_field_declaration(lexer, final, is_abstract, override, field_is_static, access, NULL, name));
			} else {
				if (access == UNDEFINED_ACCESS) {
					access = PUBLIC_ACCESS;
				}

				consume(lexer, TOKEN_IDENTIFIER, "Expected method name or variable type");

				LitToken before = lexer->previous;
				advance(lexer);

				char* type = (char *) copy_string(lexer, &before);
				char* name = (char *) copy_string(lexer, &lexer->previous);

				if (lexer->current.type == TOKEN_LEFT_PAREN) {
					if (methods == NULL) {
						methods = (LitMethods*) reallocate(lexer->compiler, NULL, 0, sizeof(LitMethods));
						lit_init_methods(methods);
					}

					lit_methods_write(MM(lexer->compiler), methods, (LitMethodStatement*) parse_method_statement(lexer, final, is_abstract, override, field_is_static, access, type, name));
				} else {
					LitToken token = lexer->previous;

					lexer->previous = before;
					lexer->current = token;
					lexer->current_code = token.start + token.length;
					lexer->line = token.line;

					if (fields == NULL) {
						fields = (LitStatements*) reallocate(lexer->compiler, NULL, 0, sizeof(LitStatements));
						lit_init_statements(fields);
					}

					advance(lexer);
					lit_statements_write(MM(lexer->compiler), fields, parse_field_declaration(lexer, final, is_abstract, override, field_is_static, access, type, name));
				}
			}
		}

		consume(lexer, TOKEN_RIGHT_BRACE, "Expect '}' after class body");
	}

	return (LitStatement*) lit_make_class_statement(lexer->compiler, line, class_name, super, methods, fields, abstract, is_static, final);
}

static LitStatement* parse_declaration(LitLexer* lexer) {
	if (match(lexer, TOKEN_VAR)) {
		return parse_var_declaration(lexer, false);
	}

	if (match(lexer, TOKEN_VAL)) {
		return parse_var_declaration(lexer, true);
	}

	if (match(lexer, TOKEN_IDENTIFIER)) {
		if (lexer->current.type != TOKEN_IDENTIFIER) {
			LitToken token = lexer->previous;

			lexer->current = token;
			lexer->current_code = token.start + token.length;
			lexer->last_line = token.line;
		} else {
			LitToken before = lexer->previous;
			advance(lexer);

			if (lexer->current.type == TOKEN_LEFT_PAREN) {
				char* type = (char *) copy_string(lexer, &before);
				char* name = (char *) copy_string(lexer, &lexer->previous);

				return parse_function_statement(lexer, type, name);
			}

			LitToken token = lexer->previous;

			lexer->previous = before;
			lexer->current = token;
			lexer->current_code = token.start + token.length;
			lexer->line = token.line;

			return parse_extended_var_declaration(lexer, &lexer->previous, &lexer->current, false);
		}
	}

	if (match(lexer, TOKEN_CLASS)) {
		return parse_class_declaration(lexer, false, false, false, true);
	}

	// TODO: a lot of mess, maybe there is a way to parse it easier?
	if (match(lexer, TOKEN_ABSTRACT)) {
		if (lexer->current.type == TOKEN_STATIC) {
			error(lexer, &lexer->current, "Abstract class can not be declared static");
			return NULL;
		} else if (lexer->current.type == TOKEN_FINAL) {
			error(lexer, &lexer->current, "Abstract class can not be declared final");
			return NULL;
		} else {
			return parse_class_declaration(lexer, true, false, false, false);
		}
	} else if (match(lexer, TOKEN_STATIC)) {
		if (lexer->current.type == TOKEN_ABSTRACT) {
			error(lexer, &lexer->current, "Static class can not be declared abstract");
			return NULL;
		} else if (lexer->current.type == TOKEN_FINAL) {
			error(lexer, &lexer->current, "Static class can not be inherited, so can it be final");
			return NULL;
		} else {
			return parse_class_declaration(lexer, false, true, false, false);
		}
	} else if (match(lexer, TOKEN_FINAL)) {
		if (match(lexer, TOKEN_IDENTIFIER)) {
			if (lexer->current.type != TOKEN_IDENTIFIER) {
				LitToken token = lexer->previous;

				lexer->current = token;
				lexer->current_code = token.start + token.length;
				lexer->last_line = token.line;
			} else {
				return parse_extended_var_declaration(lexer, &lexer->previous, &lexer->current, true);
			}
		} else if (match(lexer, TOKEN_VAR)) {
			return parse_var_declaration(lexer, true);
		} else if (match(lexer, TOKEN_VAL)) {
			error(lexer, &lexer->previous, "Val can't be declared final, because it is already final by definition");
			return NULL;
		} else {
			if (lexer->current.type == TOKEN_ABSTRACT) {
				error(lexer, &lexer->current, "Abstract class can not be declared abstract");
				return NULL;
			} else if (lexer->current.type == TOKEN_STATIC) {
				error(lexer, &lexer->current, "Final class can not be declared static");
				return NULL;
			} else {
				return parse_class_declaration(lexer, false, false, true, false);
			}
		}
	}

	return parse_statement(lexer);
}

bool lit_parse(LitCompiler* compiler, LitLexer* lexer, LitStatements* statements) {
	advance(lexer);

	if (is_at_end(lexer)) {
		error(lexer, &lexer->current, "Expected statement but got end of file");
	} else {
		while (!is_at_end(lexer)) {
			LitStatement* statement = parse_declaration(lexer);

			if (statement != NULL) {
				lit_statements_write(MM(compiler), statements, statement);
			}
		}
	}

	return lexer->had_error;
}