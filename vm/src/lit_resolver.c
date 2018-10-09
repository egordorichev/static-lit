#include <stdio.h>
#include <memory.h>

#include "lit_resolver.h"
#include "lit_table.h"
#include "lit_memory.h"
#include "lit_object.h"

DEFINE_ARRAY(LitScopes, LitLetals*, scopes)

static inline LitLetal nil_letal() {
	LitLetal letal;
  lit_init_letal(&letal);
  letal.nil = true;
  return letal;
}

DEFINE_TABLE(LitLetals, LitLetal, letals, nil_letal());

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
	LitLetals* table = (LitLetals*) reallocate(resolver->vm, NULL, 0, sizeof(LitLetals));
	lit_init_letals(table);
	lit_scopes_write(resolver->vm, &resolver->scopes, table);

	resolver->depth ++;
}

static LitLetals* peek_scope(LitResolver* resolver) {
	return resolver->scopes.values[resolver->depth - 1];
}

static void pop_scope(LitResolver* resolver) {
	if (resolver->depth == 1) {
		error(resolver, "Can't pop global scope!");
		return;
	}

	resolver->scopes.count --;
	lit_free_letals(resolver->vm, resolver->scopes.values[resolver->scopes.count]);

	resolver->depth --;
}

void lit_init_letal(LitLetal* letal) {
	letal->type = NULL;
	letal->defined = false;
	letal->nil = false;
}

static void resolve_local(LitResolver* resolver, const char* name) {
	LitString *str = lit_copy_string(resolver->vm, name, (int) strlen(name));

	for (int i = resolver->scopes.count - 1; i >= 0; i --) {
		LitLetal* value = lit_letals_get(resolver->scopes.values[i], str);

		if (!(value == NULL || value->nil)) {
			return;
		}
	}

	error(resolver, "Variable is not defined!");
}

static void declare(LitResolver* resolver, const char* name) {
	LitLetals* scope = peek_scope(resolver);
	LitString* str = lit_copy_string(resolver->vm, name, (int) strlen(name));
	LitLetal* value = lit_letals_get(scope, str);

	if (value != NULL) {
		error(resolver, "Variable with this name already exists in this scope");
	}

	value = (LitLetal*) reallocate(resolver->vm, NULL, 0, sizeof(LitLetal));

	lit_init_letal(value);
	lit_letals_set(resolver->vm, scope, str, *value);
}

static void define(LitResolver* resolver, const char* name) {
	LitLetals* scope = peek_scope(resolver);
	LitString* str = lit_copy_string(resolver->vm, name, (int) strlen(name));

	LitLetal* value = lit_letals_get(scope, str);

	if (value == NULL) {
		LitLetal *value = (LitLetal*) reallocate(resolver->vm, NULL, 0, sizeof(LitLetal));
		lit_init_letal(value);
		value->defined = true;
		lit_letals_set(resolver->vm, scope, str, *value);
	} else {
		value->defined = true;
	}
}

static void resolve_var_statement(LitResolver* resolver, LitVarStatement* statement) {
	declare(resolver, statement->name);

	if (statement->init != NULL) {
		resolve_expression(resolver, statement->init);
	}

	define(resolver, statement->name);
}

static void resolve_expression_statement(LitResolver* resolver, LitExpressionStatement* statement) {
	resolve_expression(resolver, statement->expr);
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
	push_scope(resolver);
	resolve_statements(resolver, statement->statements);
	pop_scope(resolver);
}

static void resolve_while_statement(LitResolver* resolver, LitWhileStatement* statement) {
	resolve_expression(resolver, statement->condition);
	resolve_statement(resolver, statement->body);
}

static void resolve_function(LitResolver* resolver, LitFunctionStatement* statement) {
	push_scope(resolver);

	if (statement->parameters != NULL) {
		for (int i = 0; i < statement->parameters->count; i++) {
			LitParameter parameter = statement->parameters->values[i];
			define(resolver, parameter.name);
		}
	}

	resolve_statement(resolver, statement->body);
	pop_scope(resolver);
}

static void resolve_function_statement(LitResolver* resolver, LitFunctionStatement* statement) {
	define(resolver, statement->name);
	resolve_function(resolver, statement);
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
	LitLetal* value = lit_letals_get(peek_scope(resolver), lit_copy_string(resolver->vm, expression->name, (int) strlen(expression->name)));

	if (value != NULL && !value->defined) {
		error(resolver, "Can't use local variable in it's own initializer");
	}

	resolve_local(resolver, expression->name);
}

static void resolve_assign_expression(LitResolver* resolver, LitAssignExpression* expression) {
	resolve_expression(resolver, expression->value);
	resolve_local(resolver, expression->name);
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

	// printf("%p\n", expression);

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
	resolver->depth = 0;

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