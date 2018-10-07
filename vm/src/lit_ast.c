#include "lit_ast.h"
#include "lit_memory.h"

#define ALLOCATE_AST(vm, type, object_type) \
    (type*) allocate_ast(vm, sizeof(type), object_type)

static LitExpression* allocate_ast(LitVm* vm, size_t size, LitExpresionType type) {
	LitExpression* object = (LitExpression*) reallocate(vm, NULL, 0, size);
	object->type = type;
	return object;
}

LitBinaryExpression* lit_make_binary_expression(LitVm* vm, LitExpression* left, LitExpression* right, LitTokenType operator) {
	LitBinaryExpression* expression = ALLOCATE_AST(vm, LitBinaryExpression, BINARY_AST);

	expression->left = left;
	expression->right = right;
	expression->operator = operator;

	return expression;
}

LitLiteralExpression* lit_make_literal_expression(LitVm* vm, LitValue value) {
	LitLiteralExpression* expression = ALLOCATE_AST(vm, LitLiteralExpression, LITERAL_AST);

	expression->value = value;

	return expression;
}

LitUnaryExpression* lit_make_unary_expression(LitVm* vm, LitExpression* right, LitTokenType type) {
	LitUnaryExpression* expression = ALLOCATE_AST(vm, LitUnaryExpression, UNARY_AST);

	expression->right = right;
	expression->operator = type;

	return expression;
}


LitGroupingExpression* lit_make_grouping_expression(LitVm* vm, LitExpression* expr) {
	LitGroupingExpression* expression = ALLOCATE_AST(vm, LitGroupingExpression, GROUPING_AST);

	expression->expr = expr;

	return expression;
}

LitStatementExpression* lit_make_statement_expression(LitVm* vm, LitExpression* expr) {
	LitStatementExpression* expression = ALLOCATE_AST(vm, LitStatementExpression, STATEMENT_AST);

	expression->expr = expr;

	return expression;
}