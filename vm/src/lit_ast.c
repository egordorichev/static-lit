#include <stdio.h>

#include "lit_ast.h"
#include "lit_memory.h"

#define ALLOCATE_EXPRESSION(vm, type, object_type) \
    (type*) allocate_expression(vm, sizeof(type), object_type)

static LitExpression* allocate_expression(LitVm* vm, size_t size, LitExpresionType type) {
	LitExpression* object = (LitExpression*) reallocate(vm, NULL, 0, size);
	object->type = type;
	return object;
}

#define ALLOCATE_STATEMENT(vm, type, object_type) \
    (type*) allocate_statement(vm, sizeof(type), object_type)

static LitStatement* allocate_statement(LitVm* vm, size_t size, LitStatementType type) {
	LitStatement* object = (LitStatement*) reallocate(vm, NULL, 0, size);
	object->type = type;
	return object;
}

LitBinaryExpression* lit_make_binary_expression(LitVm* vm, LitExpression* left, LitExpression* right, LitTokenType operator) {
	LitBinaryExpression* expression = ALLOCATE_EXPRESSION(vm, LitBinaryExpression, BINARY_EXPRESSION);

	expression->left = left;
	expression->right = right;
	expression->operator = operator;

	return expression;
}

LitLiteralExpression* lit_make_literal_expression(LitVm* vm, LitValue value) {
	LitLiteralExpression* expression = ALLOCATE_EXPRESSION(vm, LitLiteralExpression, LITERAL_EXPRESSION);

	expression->value = value;

	return expression;
}

LitUnaryExpression* lit_make_unary_expression(LitVm* vm, LitExpression* right, LitTokenType type) {
	LitUnaryExpression* expression = ALLOCATE_EXPRESSION(vm, LitUnaryExpression, UNARY_EXPRESSION);

	expression->right = right;
	expression->operator = type;

	return expression;
}


LitGroupingExpression* lit_make_grouping_expression(LitVm* vm, LitExpression* expr) {
	LitGroupingExpression* expression = ALLOCATE_EXPRESSION(vm, LitGroupingExpression, GROUPING_EXPRESSION);

	expression->expr = expr;

	return expression;
}

LitVarStatement* lit_make_var_statement(LitVm* vm, LitToken* name, LitExpression* init) {
	LitVarStatement* statement = ALLOCATE_STATEMENT(vm, LitVarStatement, VAR_STATEMENT);

	statement->name = name;
	statement->init = init;

	return statement;
}

LitExpressionStatement* lit_make_expression_statement(LitVm* vm, LitExpression* expr) {
	LitExpressionStatement* statement = ALLOCATE_STATEMENT(vm, LitExpressionStatement, EXPRESSION_STATEMENT);

	statement->expr = expr;

	return statement;
}