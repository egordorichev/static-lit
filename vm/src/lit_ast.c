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