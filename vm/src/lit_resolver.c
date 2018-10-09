#include <stdio.h>
#include <memory.h>

#include "lit_resolver.h"
#include "lit_table.h"
#include "lit_memory.h"
#include "lit_object.h"

DEFINE_ARRAY(LitScopes, LitTable*, scopes)

/*
 * TODO: type work: ensure argument types, ensure argument types are defined
 */

static void resolve_statement(LitResolver* resolver, LitStatement* statement);
static void resolve_statements(LitResolver* resolver, LitStatements* statements);

static void resolve_expression(LitResolver* resolver, LitExpression* expression);
static void resolve_expressions(LitResolver* resolver, LitExpressions* expressions);

static void error(LitResolver* resolver, const char* message) {
	resolver->had_error = true;
	printf("Error: %s\n", message);
}

static void push_scope(LitResolver* resolver) {
	LitTable* table = (LitTable*) reallocate(resolver->vm, NULL, 0, sizeof(LitTable));
	lit_init_table(table);
	lit_scopes_write(resolver->vm, &resolver->scopes, table);

	resolver->depth ++;
}

static LitTable* peek_scope(LitResolver* resolver) {
	return resolver->scopes.values[resolver->depth - 1];
}

static void pop_scope(LitResolver* resolver) {
	if (resolver->depth == 1) {
		error(resolver, "Can't pop global scope!");
		return;
	}

	resolver->scopes.count --;
	lit_free_table(resolver->vm, resolver->scopes.values[resolver->scopes.count]);

	resolver->depth --;
}

static void resolve_local(LitResolver* resolver, const char* name, int len) {
	LitString *str = lit_copy_string(resolver->vm, name, len);

	for (int i = resolver->scopes.count - 1; i >= 0; i --) {
		LitValue value;
		lit_table_get(resolver->scopes.values[i], str, &value);

		if (value != NIL_VALUE) {
			// TODO: set it! http://craftinginterpreters.com/resolving-and-binding.html
			return;
		}
	}

	error(resolver, "Variable is not defined!");
}

static void declare(LitResolver* resolver, const char* name, int len) {
	lit_table_set(resolver->vm, peek_scope(resolver), lit_copy_string(resolver->vm, name, len), FALSE_VALUE);
}

static void define(LitResolver* resolver, const char* name, int len) {
	lit_table_set(resolver->vm, peek_scope(resolver), lit_copy_string(resolver->vm, name, len), TRUE_VALUE);
}

static void resolve_var_statement(LitResolver* resolver, LitVarStatement* statement) {
	declare(resolver, statement->name->start, statement->name->length);

	if (statement->init != NULL) {
		resolve_expression(resolver, statement->init);
	}

	define(resolver, statement->name->start, statement->name->length);
}

static void resolve_expression_statement(LitResolver* resolver, LitExpressionStatement* statement) {
	resolve_expression(resolver, statement->expression);
}

static void resolve_if_statement(LitResolver* resolver, LitIfStatement* statement) {
	resolve_expression(resolver, statement->condition);
	resolve_statement(resolver, statement->if_branch);

	if (statement->else_if_branches != NULL) {
		resolve_expressions(resolver, statement->else_if_conditions);
		resolve_statements(resolver, statement->else_if_branches);
	}

	if (statement->else_branch != NULL) {
		resolve_statement(resolver, statement->else_branch);
	}
}

static void resolve_block_statement(LitResolver* resolver, LitBlockStatement* statement) {
	resolve_statements(resolver, statement->statements);
}

static void resolve_while_statement(LitResolver* resolver, LitWhileStatement* statement) {
	resolve_expression(resolver, statement->condition);
	resolve_statement(resolver, statement->body);
}

static void resolve_function_statement(LitResolver* resolver, LitFunctionStatement* statement) {
	resolve_statement(resolver, statement->body);
}

static void resolve_return_statement(LitResolver* resolver, LitReturnStatement* statement) {
	resolve_expression(resolver, statement->value);
}

static void resolve_statement(LitResolver* resolver, LitStatement* statement) {
	if (statement == NULL) {
		return;
	}

	switch (statement->type) {
		case VAR_STATEMENT: resolve_var_statement(resolver, (LitVarStatement*) statement); break;
		case EXPRESSION_STATEMENT: resolve_expression_statement(resolver, (LitExpressionStatement*) statement); break;
		case IF_STATEMENT: resolve_if_statement(resolver, (LitIfStatement*) statement); break;
		case BLOCK_STATEMENT: resolve_block_statement(resolver, (LitBlockStatement*) statement); break;
		case WHILE_STATEMENT: resolve_while_statement(resolver, (LitWhileStatement*) statement); break;
		case FUNCTION_STATEMENT: resolve_function_statement(resolver, (LitFunctionStatement*) statement); break;
		case RETURN_STATEMENT: resolve_return_statement(resolver, (LitReturnStatement*) statement); break;
	}
}

static void resolve_statements(LitResolver* resolver, LitStatements* statements) {
	for (int i = 0; i < statements->count; i++) {
		resolve_statement(resolver, statements->values[i]);
	}
}

static void resolve_binary_expression(LitResolver* resolver, LitBinaryExpression* expression) {
	resolve_expression(resolver, expression->left);
	resolve_expression(resolver, expression->right);
}

static void resolve_literal_expression(LitResolver* resolver, LitLiteralExpression* expression) {

}

static void resolve_unary_expression(LitResolver* resolver, LitUnaryExpression* expression) {
	resolve_expression(resolver, expression->right);
}

static void resolve_grouping_expression(LitResolver* resolver, LitGroupingExpression* expression) {
	resolve_expression(resolver, expression->expr);
}

static void resolve_var_expression(LitResolver* resolver, LitVarExpression* expression) {
	LitValue value;

	printf("%s\n", expression->name->start);

	lit_table_get(peek_scope(resolver), lit_copy_string(resolver->vm, expression->name->start, expression->name->length), &value);

	if (value == FALSE_VALUE) {
		error(resolver, "Can't use local variable in it's own initializer");
	}

	// resolve_local(resolver, expression->name->start, expression->name->length);
}

static void resolve_assign_expression(LitResolver* resolver, LitAssignExpression* expression) {
	resolve_expression(resolver, expression->value);
}

static void resolve_logical_expression(LitResolver* resolver, LitLogicalExpression* expression) {
	resolve_expression(resolver, expression->right);
}

static void resolve_call_expression(LitResolver* resolver, LitCallExpression* expression) {
	resolve_expression(resolver, expression->callee);
	resolve_expressions(resolver, expression->args);
}

static void resolve_expression(LitResolver* resolver, LitExpression* expression) {
	if (expression == NULL) {
		return;
	}

	switch (expression->type) {
		case BINARY_EXPRESSION: resolve_binary_expression(resolver, (LitBinaryExpression*) expression); break;
		case LITERAL_EXPRESSION: resolve_literal_expression(resolver, (LitLiteralExpression*) expression); break;
		case UNARY_EXPRESSION: resolve_unary_expression(resolver, (LitUnaryExpression*) expression); break;
		case GROUPING_EXPRESSION: resolve_grouping_expression(resolver, (LitGroupingExpression*) expression); break;
		case VAR_EXPRESSION: resolve_var_expression(resolver, (LitVarExpression*) expression); break;
		case ASSIGN_EXPRESSION: resolve_assign_expression(resolver, (LitAssignExpression*) expression); break;
		case LOGICAL_EXPRESSION: resolve_logical_expression(resolver, (LitLogicalExpression*) expression); break;
		case CALL_EXPRESSION: resolve_call_expression(resolver, (LitCallExpression*) expression); break;
	}
}

static void resolve_expressions(LitResolver* resolver, LitExpressions* expressions) {
	for (int i = 0; i < expressions->count; i++) {
		resolve_expression(resolver, expressions->values[i]);
	}
}

void lit_init_resolver(LitResolver* resolver) {
	lit_init_scopes(&resolver->scopes);
	resolver->had_error = false;

	push_scope(resolver); // Global scope
}

void lit_free_resolver(LitResolver* resolver) {
	lit_free_scopes(resolver->vm, &resolver->scopes);
}

bool lit_resolve(LitVm* vm, LitStatements* statements) {
	LitResolver resolver;
	resolver.vm = vm;

	lit_init_resolver(&resolver);
	resolve_statements(&resolver, statements);
	lit_free_resolver(&resolver);

	return resolver.had_error;
}