#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include <lit.h>
#include <lit_debug.h>
#include <compiler/lit_parser.h>
#include <vm/lit_memory.h>

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

static const char* copy_string_native(LitLexer* lexer, const char* name, int length) {
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

	fflush(stdout);
	fprintf(stderr, "[line %d] Error", token->line);

	if (token->type == TOKEN_EOF) {
		fprintf(stderr, " at end");
	} else if (token->type != TOKEN_ERROR) {
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);
	fflush(stderr);
}

static LitToken consume(LitLexer* lexer, LitTokenType type, const char* message) {
	if (!match(lexer, type)) {
		error(lexer, &lexer->current, message);
		return lexer->current;
	}

	return lexer->previous;
}

static LitExpression* parse_lambda(LitLexer* lexer);

static LitExpression* parse_primary(LitLexer* lexer) {
	if (match(lexer, TOKEN_FUN)) {
		return parse_lambda(lexer);
	}

	if (match(lexer, TOKEN_THIS)) {
		return (LitExpression*) lit_make_this_expression(lexer->compiler);
	}

	if (match(lexer, TOKEN_SUPER)) {
		consume(lexer, TOKEN_DOT, "Expected '.' after super");
		LitToken token = consume(lexer, TOKEN_IDENTIFIER, "Expected method name after dot");

		return (LitExpression*) lit_make_super_expression(lexer->compiler, copy_string(lexer, &token));
	}

	if (match(lexer, TOKEN_IDENTIFIER)) {
		return (LitExpression*) lit_make_var_expression(lexer->compiler, copy_string(lexer, &lexer->previous));
	}

	if (match(lexer, TOKEN_TRUE)) {
		return (LitExpression*) lit_make_literal_expression(lexer->compiler, TRUE_VALUE);
	}

	if (match(lexer, TOKEN_FALSE)) {
		return (LitExpression*) lit_make_literal_expression(lexer->compiler, FALSE_VALUE);
	}

	if (match(lexer, TOKEN_NIL)) {
		return (LitExpression*) lit_make_literal_expression(lexer->compiler, NIL_VALUE);
	}

	if (match(lexer, TOKEN_NUMBER)) {
		return (LitExpression*) lit_make_literal_expression(lexer->compiler, MAKE_NUMBER_VALUE(strtod(lexer->previous.start, NULL)));
	}

	if (match(lexer, TOKEN_STRING)) {
		return (LitExpression*) lit_make_literal_expression(lexer->compiler,
			MAKE_OBJECT_VALUE(lit_copy_string(lexer->compiler, lexer->previous.start + 1, lexer->previous.length - 2)));
	}

	if (match(lexer, TOKEN_CHAR)) {
		return (LitExpression*) lit_make_literal_expression(lexer->compiler, MAKE_CHAR_VALUE(lexer->previous.start[1]));
	}

	if (match(lexer, TOKEN_LEFT_PAREN)) {
		LitExpression* expression = parse_expression(lexer);
		consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
		return (LitExpression*) lit_make_grouping_expression(lexer->compiler, expression);
	}

	error(lexer, &lexer->current, "Unexpected token");
	advance(lexer);

	return parse_primary(lexer);
}

static LitExpression* finish_call(LitLexer* lexer, LitExpression* callee) {
	LitExpressions* args = (LitExpressions*) reallocate(lexer->compiler, NULL, 0, sizeof(LitExpressions));
	lit_init_expressions(args);

	if (lexer->current.type != TOKEN_RIGHT_PAREN) {
		do {
			lit_expressions_write(lexer->compiler, args, parse_expression(lexer));
		} while (match(lexer, TOKEN_COMMA));
	}

	consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after arguments");
	return (LitExpression*) lit_make_call_expression(lexer->compiler, callee, args);
}

static LitExpression* parse_equality(LitLexer* lexer);

static LitExpression* parse_call(LitLexer* lexer) {
	LitExpression* expression = parse_primary(lexer);

	while (true) {
		if (match(lexer, TOKEN_LEFT_PAREN)) {
			expression = finish_call(lexer, expression);
		} else if (match(lexer, TOKEN_DOT)) {
			LitToken token = consume(lexer, TOKEN_IDENTIFIER, "Expected property name after '.'");

			if (match(lexer, TOKEN_EQUAL)) {
				expression = (LitExpression*) lit_make_set_expression(lexer->compiler, expression, parse_equality(lexer), copy_string(lexer, &token));
			} else {
				expression = (LitExpression*) lit_make_get_expression(lexer->compiler, expression, copy_string(lexer, &token));
			}
		} else {
			break;
		}
	}

	return expression;
}

static LitExpression* parse_unary(LitLexer* lexer) {
	if (match(lexer, TOKEN_BANG) || match(lexer, TOKEN_MINUS)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression* right = parse_unary(lexer);
		return (LitExpression*) lit_make_unary_expression(lexer->compiler, right, operator);
	}

	return parse_call(lexer);
}

static LitExpression* parse_multiplication(LitLexer* lexer) {
	LitExpression *expression = parse_unary(lexer);

	while (match(lexer, TOKEN_STAR) || match(lexer, TOKEN_SLASH)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression *right = parse_unary(lexer);

		expression = (LitExpression*) lit_make_binary_expression(lexer->compiler, expression, right, operator);
	}

	return expression;
}

static LitExpression* parse_addition(LitLexer* lexer) {
	LitExpression *expression = parse_multiplication(lexer);

	while (match(lexer, TOKEN_PLUS) || match(lexer, TOKEN_MINUS)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression *right = parse_multiplication(lexer);

		expression = (LitExpression*) lit_make_binary_expression(lexer->compiler, expression, right, operator);
	}

	return expression;
}

static LitExpression* parse_comprasion(LitLexer *lexer) {
	LitExpression *expression = parse_addition(lexer);

	while (match(lexer, TOKEN_LESS) || match(lexer, TOKEN_LESS_EQUAL) || match(lexer, TOKEN_GREATER) || match(lexer, TOKEN_GREATER_EQUAL)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression *right = parse_addition(lexer);

		expression = (LitExpression*) lit_make_binary_expression(lexer->compiler, expression, right, operator);
	}

	return expression;
}

static LitExpression* parse_equality(LitLexer* lexer) {
	LitExpression *expression = parse_comprasion(lexer);

	while (match(lexer, TOKEN_BANG_EQUAL) || match(lexer, TOKEN_EQUAL_EQUAL)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression* right = parse_comprasion(lexer);

		expression = (LitExpression*) lit_make_binary_expression(lexer->compiler, expression, right, operator);
	}

	return expression;
}

static LitExpression* parse_and(LitLexer* lexer) {
	LitExpression* expression = parse_equality(lexer);

	while (match(lexer, TOKEN_AND)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression* right = parse_equality(lexer);

		expression = (LitExpression*) lit_make_logical_expression(lexer->compiler, operator, expression, right);
	}

	return expression;
}

static LitExpression* parse_or(LitLexer* lexer) {
	LitExpression* expression = parse_and(lexer);

	while (match(lexer, TOKEN_OR)) {
		LitTokenType operator = lexer->previous.type;
		LitExpression* right = parse_and(lexer);

		expression = (LitExpression*) lit_make_logical_expression(lexer->compiler, operator, expression, right);
	}

	return expression;
}

static LitExpression* parse_assigment(LitLexer* lexer) {
	LitExpression* expression = parse_or(lexer);

	if (match(lexer, TOKEN_EQUAL)) {
		LitToken equal = lexer->previous;
		LitExpression* value = parse_assigment(lexer);

		if (expression->type == VAR_EXPRESSION) {
			return (LitExpression*) lit_make_assign_expression(lexer->compiler, expression, value);
		}

		error(lexer, &equal, "Invalid assigment target");
	}

	return expression;
}

static LitExpression* parse_expression(LitLexer* lexer) {
	return parse_assigment(lexer);
}

static LitStatement* parse_expression_statement(LitLexer* lexer) {
	return (LitStatement*) lit_make_expression_statement(lexer->compiler, parse_expression(lexer));
}

static LitStatement* parse_if_statement(LitLexer* lexer) {
	consume(lexer, TOKEN_LEFT_PAREN, "Expected '(' after if");
	LitExpression* condition = parse_expression(lexer);
	consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after if condition");

	LitStatement* if_branch = parse_statement(lexer);
	LitStatement* else_branch = NULL;
	LitStatements* else_if_branches = NULL;
	LitExpressions* else_if_conditions = NULL;

	while (match(lexer, TOKEN_ELSE)) {
		if (match(lexer, TOKEN_IF)) {
			if (else_branch != NULL) {
				error(lexer, &lexer->current, "Else branch is already defined for this if");
			}

			if (else_if_branches == NULL) {
				else_if_conditions = (LitExpressions*) reallocate(lexer->compiler, NULL, 0, sizeof(LitExpressions));
				lit_init_expressions(else_if_conditions);

				else_if_branches = (LitStatements*) reallocate(lexer->compiler, NULL, 0, sizeof(LitStatements));
				lit_init_statements(else_if_branches);
			}

			consume(lexer, TOKEN_LEFT_PAREN, "Expected '(' after else if");
			lit_expressions_write(lexer->compiler, else_if_conditions, parse_expression(lexer));
			consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after else if condition");

				lit_statements_write(lexer->compiler, else_if_branches, parse_statement(lexer));
		} else {
			else_branch = parse_statement(lexer);
		}
	}

	return (LitStatement*) lit_make_if_statement(lexer->compiler, condition, if_branch, else_branch, else_if_branches, else_if_conditions);
}

static LitStatement* parse_while(LitLexer* lexer) {
	consume(lexer, TOKEN_LEFT_PAREN, "Expected '(' after while");
	LitExpression* condition = parse_expression(lexer);
	consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after while condition");

	return (LitStatement*) lit_make_while_statement(lexer->compiler, condition, parse_statement(lexer));
}

static LitStatement* parse_for(LitLexer* lexer) {
	consume(lexer, TOKEN_LEFT_PAREN, "Expected '(' after while");

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

	consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after for");

	LitStatement* body = parse_statement(lexer);

	if (increment != NULL) {
		LitStatements* statements = (LitStatements*) reallocate(lexer->compiler, NULL, 0, sizeof(LitStatements));
		lit_init_statements(statements);

		lit_statements_write(lexer->compiler, statements, body);
		lit_statements_write(lexer->compiler, statements, (LitStatement*) lit_make_expression_statement(lexer->compiler, increment));

		body = (LitStatement*) lit_make_block_statement(lexer->compiler, statements);
	}

	if (condition == NULL) {
		condition = (LitExpression*) lit_make_literal_expression(lexer->compiler, TRUE_VALUE);
	}

	body = (LitStatement*) lit_make_while_statement(lexer->compiler, condition, body);

	if (init != NULL) {
		LitStatements* statements = (LitStatements*) reallocate(lexer->compiler, NULL, 0, sizeof(LitStatements));
		lit_init_statements(statements);

		lit_statements_write(lexer->compiler, statements, init);
		lit_statements_write(lexer->compiler, statements, body);

		body = (LitStatement*) lit_make_block_statement(lexer->compiler, statements);
	}

	return body;
}

static LitStatement* parse_block_statement(LitLexer* lexer) {
	LitStatements* statements = NULL;

	while (!match(lexer, TOKEN_RIGHT_BRACE)) {
		if (statements == NULL) {
			statements = (LitStatements*) reallocate(lexer->compiler, NULL, 0, sizeof(LitStatements));
			lit_init_statements(statements);
		}

		if (lexer->current.type == TOKEN_EOF) {
			error(lexer, &lexer->current, "Expected '}' to close the block");
			break;
		}

		lit_statements_write(lexer->compiler, statements, parse_declaration(lexer));
	}

	return (LitStatement*) lit_make_block_statement(lexer->compiler, statements);
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

	return (char*) copy_string_native(lexer, start, (int) (lexer->current.start - start - 1));
}

static LitExpression* parse_lambda(LitLexer* lexer) {
	consume(lexer, TOKEN_LEFT_PAREN, "Expected '(' after 'fun'");

	LitParameters* parameters = NULL;

	if (lexer->current.type != TOKEN_RIGHT_PAREN) {
		parameters = (LitParameters*) reallocate(lexer->compiler, NULL, 0, sizeof(LitParameters));
		lit_init_parameters(parameters);

		do {
			char* type = parse_argument_type(lexer);
			LitToken name = consume(lexer, TOKEN_IDENTIFIER, "Expected argument name");

			lit_parameters_write(lexer->compiler, parameters, (LitParameter) {copy_string_native(lexer, name.start, name.length), type});
		} while (match(lexer, TOKEN_COMMA));
	}

	consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");

	LitParameter return_type = (LitParameter) {NULL, "void"};

	if (match(lexer, TOKEN_GREATER)) {
		LitToken type = consume(lexer, TOKEN_IDENTIFIER, "Expected return type");

		return_type.type = copy_string(lexer, &type);
	}

	consume(lexer, TOKEN_LEFT_BRACE, "Expected '{' before lambda body");
	return (LitExpression*) lit_make_lambda_expression(lexer->compiler, parameters, parse_block_statement(lexer), (LitParameter) {NULL, return_type.type});
}

static LitStatement* parse_function_statement(LitLexer* lexer) {
	LitToken function_name = consume(lexer, TOKEN_IDENTIFIER, "Expected function name");
	consume(lexer, TOKEN_LEFT_PAREN, "Expected '(' after function name");

	LitParameters* parameters = NULL;

	if (lexer->current.type != TOKEN_RIGHT_PAREN) {
		parameters = (LitParameters*) reallocate(lexer->compiler, NULL, 0, sizeof(LitParameters));
		lit_init_parameters(parameters);

		do {
			char* type = parse_argument_type(lexer);
			LitToken name = consume(lexer, TOKEN_IDENTIFIER, "Expected argument name");

			lit_parameters_write(lexer->compiler, parameters, (LitParameter) {copy_string_native(lexer, name.start, name.length), type});
		} while (match(lexer, TOKEN_COMMA));
	}

	consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");

	LitParameter return_type = (LitParameter) {NULL, "void"};

	if (match(lexer, TOKEN_GREATER)) {
		LitToken type = consume(lexer, TOKEN_IDENTIFIER, "Expected return type");

		return_type.type = copy_string(lexer, &type);
	}

	LitBlockStatement* body;

	if (match(lexer, TOKEN_EQUAL)) {
		consume(lexer, TOKEN_GREATER, "Expected '>' after '='");
		LitStatements* statements = (LitStatements*) reallocate(lexer->compiler, NULL, 0, sizeof(LitStatements));

		lit_init_statements(statements);
		lit_statements_write(lexer->compiler, statements, parse_statement(lexer));

		body = lit_make_block_statement(lexer->compiler, statements);
	} else {
		consume(lexer, TOKEN_LEFT_BRACE, "Expected '{' before function body");
		body = (LitBlockStatement*) parse_block_statement(lexer);
	}

	return (LitStatement*) lit_make_function_statement(lexer->compiler, copy_string(lexer, &function_name), parameters, (LitStatement*) body, (LitParameter) {NULL, return_type.type});
}

static LitStatement* parse_method_statement(LitLexer* lexer, bool final, bool abstract, bool override, bool is_static, LitAccessType access, LitToken* method_name) {
	const char* name = copy_string(lexer, method_name);

	if (final) {
		error(lexer, &lexer->previous, "Can't declare a final method");
	}

	if (is_static && override) {
		error(lexer, &lexer->previous, "Can't override a static class");
	}

	consume(lexer, TOKEN_LEFT_PAREN, "Expected '(' after method name");

	LitParameters* parameters = NULL;

	if (lexer->current.type != TOKEN_RIGHT_PAREN) {
		parameters = (LitParameters*) reallocate(lexer->compiler, NULL, 0, sizeof(LitParameters));
		lit_init_parameters(parameters);

		do {
			char* type = parse_argument_type(lexer);
			LitToken name = consume(lexer, TOKEN_IDENTIFIER, "Expected argument name");

			lit_parameters_write(lexer->compiler, parameters, (LitParameter) {copy_string_native(lexer, name.start, name.length), type});
		} while (match(lexer, TOKEN_COMMA));
	}

	consume(lexer, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");

	LitParameter return_type = (LitParameter) {NULL, "void"};

	if (match(lexer, TOKEN_GREATER)) {
		LitToken type = consume(lexer, TOKEN_IDENTIFIER, "Expected return type");

		return_type.type = copy_string(lexer, &type);
	}

	LitBlockStatement* body = NULL;

	if (match(lexer, TOKEN_LEFT_BRACE)) {
		if (abstract) {
			error(lexer, &lexer->previous, "Abstract method can't have body");
		} else {
			body = (LitBlockStatement*) parse_block_statement(lexer);
		}
	} else if (match(lexer, TOKEN_EQUAL)) {
		consume(lexer, TOKEN_GREATER, "Expected '>' after '='");

		if (abstract) {
			error(lexer, &lexer->previous, "Abstract method can't have body");
		} else {
			LitStatements* statements = (LitStatements*) reallocate(lexer->compiler, NULL, 0, sizeof(LitStatements));

			lit_init_statements(statements);
			lit_statements_write(lexer->compiler, statements, (LitStatement*) lit_make_return_statement(lexer->compiler, parse_expression(lexer)));

			body = lit_make_block_statement(lexer->compiler, statements);
		}
	}

	return (LitStatement*) lit_make_method_statement(lexer->compiler, name, parameters, (LitStatement*) body, (LitParameter) {NULL, return_type.type}, override, is_static, abstract, access);
}

static LitStatement* parse_return_statement(LitLexer* lexer) {
	return (LitStatement*) lit_make_return_statement(lexer->compiler, (lexer->current.type != TOKEN_RIGHT_BRACE && lexer->current.type != TOKEN_EOF) ? parse_expression(lexer) : NULL);
}

static LitStatement* parse_statement(LitLexer* lexer) {
	if (match(lexer, TOKEN_LEFT_BRACE)) {
		return parse_block_statement(lexer);
	}

	if (match(lexer, TOKEN_IF)) {
		return parse_if_statement(lexer);
	}

	if (match(lexer, TOKEN_FUN)) {
		return parse_function_statement(lexer);
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
		return (LitStatement *) lit_make_break_statement(lexer->compiler);
	}

	if (match(lexer, TOKEN_CONTINUE)) {
		return (LitStatement *) lit_make_continue_statement(lexer->compiler);
	}

	return parse_expression_statement(lexer);
}

static LitStatement* parse_var_declaration(LitLexer* lexer, bool final) {
	LitToken name = consume(lexer, TOKEN_IDENTIFIER, "Expected variable name");
	LitExpression* init = NULL;

	if (match(lexer, TOKEN_EQUAL)) {
		init = parse_expression(lexer);
	}

	return (LitStatement*) lit_make_var_statement(lexer->compiler, copy_string(lexer, &name), init, NULL, final);
}

static LitStatement* parse_extended_var_declaration(LitLexer* lexer, LitToken* type, LitToken* name, bool final) {
	LitExpression* init = NULL;

	if (match(lexer, TOKEN_EQUAL)) {
		init = parse_expression(lexer);
	}

	return (LitStatement*) lit_make_var_statement(lexer->compiler, copy_string(lexer, name), init, copy_string(lexer, type), final);
}

static LitStatement* parse_field_declaration(LitLexer* lexer, bool final, bool abstract, bool override, bool is_static, LitAccessType access, LitToken* type, LitToken* var_name) {
	LitExpression* init = NULL;
	const char* name = copy_string(lexer, var_name);

	if (abstract) {
		error(lexer, var_name, "Can't declare abstract field");
	}

	if (override) {
		error(lexer, var_name, "Can't override a field");
	}

	if (match(lexer, TOKEN_EQUAL)) {
		init = parse_expression(lexer);
	}

	if (final && init == NULL) {
		error(lexer, &lexer->previous, "Can't declare a final variable without value");
	}

	if (type == NULL && init == NULL) {
		error(lexer, &lexer->previous, "Can't declare a field without type");
	}

	return (LitStatement*) lit_make_field_statement(lexer->compiler, name, init, type == NULL ? NULL : copy_string(lexer, type), NULL, NULL, access, is_static, final);
}

static LitStatement* parse_class_declaration(LitLexer* lexer, bool abstract, bool is_static, bool final, bool had_class_token) {
	if (!had_class_token) {
		consume(lexer, TOKEN_CLASS, "Expected 'class'");
	}

	LitToken name = consume(lexer, TOKEN_IDENTIFIER, "Expect class name");
	LitVarExpression* super = NULL;

	if (match(lexer, TOKEN_LESS)) {
		if (is_static) {
			error(lexer, &lexer->previous, "Static classes can't inherit other classes");
		}

		consume(lexer, TOKEN_IDENTIFIER, "Expected superclass name");
		super = lit_make_var_expression(lexer->compiler, copy_string(lexer, &lexer->previous));
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
						} else {
							final = true;
						}

						break;
					}
					case TOKEN_STATIC: {
						if (field_is_static) {
							error(lexer, &lexer->previous, is_static ? "No need to use static field modifier in a static class" : "Static keyword used twice or more per field");
						}

						field_is_static = true;
						break;
					}
					case TOKEN_PRIVATE: {
						if (access != UNDEFINED_ACCESS) {
							error(lexer, &lexer->previous, "Access modifier used twice or more per field");
						}
	
						access = PRIVATE_ACCESS;
						break;
					}
					case TOKEN_PROTECTED: {
						if (access != UNDEFINED_ACCESS) {
							error(lexer, &lexer->previous, "Access modifier used twice or more per field");
						}
	
						access = PROTECTED_ACCESS;
						break;
					}
					case TOKEN_PUBLIC: {
						if (access != UNDEFINED_ACCESS) {
							error(lexer, &lexer->previous, "Access modifier used twice or more per field");
						}
	
						access = PUBLIC_ACCESS;
						break;
					}
					case TOKEN_OVERRIDE: {
						if (override) {
							error(lexer, &lexer->previous, "Override keyword used twice or more per field");
						}

						override = true;
						break;
					}
					case TOKEN_ABSTRACT: {
						if (is_abstract) {
							error(lexer, &lexer->previous, "Abstract keyword used twice or more per field");
						}

						is_abstract = true;
						break;
					}
				}
			}

			if (access == UNDEFINED_ACCESS) {
				access = PUBLIC_ACCESS;
			}

			if (match(lexer, TOKEN_VAR)) {
				if (fields == NULL) {
					fields = (LitStatements*) reallocate(lexer->compiler, NULL, 0, sizeof(LitStatements));
					lit_init_statements(fields);
				}

				LitToken* name = &lexer->previous;
				advance(lexer);
				lit_statements_write(lexer->compiler, fields, parse_field_declaration(lexer, final, is_abstract, override, field_is_static, access, NULL, name));
			} else {
				consume(lexer, TOKEN_IDENTIFIER, "Expected method name or variable type");

				if (lexer->current.type != TOKEN_IDENTIFIER) {
					if (methods == NULL) {
						methods = (LitMethods*) reallocate(lexer->compiler, NULL, 0, sizeof(LitMethods));
						lit_init_methods(methods);
					}

					lit_methods_write(lexer->compiler, methods, (LitMethodStatement*) parse_method_statement(lexer, final, is_abstract, override, field_is_static, access, &lexer->previous));
				} else {
					LitToken type = lexer->previous;

					if (fields == NULL) {
						fields = (LitStatements*) reallocate(lexer->compiler, NULL, 0, sizeof(LitStatements));
						lit_init_statements(fields);
					}

					LitToken* name = &lexer->previous;
					advance(lexer);
					lit_statements_write(lexer->compiler, fields, parse_field_declaration(lexer, final, is_abstract, override, field_is_static, access, &type, name));
				}
			}
		}

		consume(lexer, TOKEN_RIGHT_BRACE, "Expect '}' after class body");
	}

	return (LitStatement*) lit_make_class_statement(lexer->compiler, copy_string(lexer, &name), super, methods, fields, abstract, is_static, final);
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
			lexer->line = token.line;
		} else {
			LitToken type = lexer->previous;
			// TODO: final int test = 10
			return parse_extended_var_declaration(lexer, &type, &lexer->current, false);
		}
	}

	if (match(lexer, TOKEN_CLASS)) {
		return parse_class_declaration(lexer, false, false, false, true);
	}

	// TODO: a lot of mess, maybe there is a way to parse it easier?
	if (match(lexer, TOKEN_ABSTRACT)) {
		if (lexer->current.type == TOKEN_STATIC) {
			error(lexer, &lexer->current, "Abstract class can not be declared static");
		} else if (lexer->current.type == TOKEN_FINAL) {
			error(lexer, &lexer->current, "Abstract class can not be declared final");
		} else {
			return parse_class_declaration(lexer, true, false, false, false);
		}
	} else if (match(lexer, TOKEN_STATIC)) {
		if (lexer->current.type == TOKEN_ABSTRACT) {
			error(lexer, &lexer->current, "Static class can not be declared abstract");
		} else if (lexer->current.type == TOKEN_FINAL) {
			error(lexer, &lexer->current, "Static class can not be inherited, so can it be final");
		} else {
			return parse_class_declaration(lexer, false, true, false, false);
		}
	} else if (match(lexer, TOKEN_FINAL)) {
		if (match(lexer, TOKEN_VAR)) {
			return parse_var_declaration(lexer, true);
		} else if (match(lexer, TOKEN_VAL)) {
			error(lexer, &lexer->previous, "Val can't be declared final, because it is already final by definition");
			return parse_var_declaration(lexer, true);
		} else {
			if (lexer->current.type == TOKEN_ABSTRACT) {
				error(lexer, &lexer->current, "Abstract class can not be declared abstract");
			} else if (lexer->current.type == TOKEN_STATIC) {
				error(lexer, &lexer->current, "Final class can not be declared static");
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
			lit_statements_write(compiler, statements, parse_declaration(lexer));
		}
	}

	return lexer->had_error;
}