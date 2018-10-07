#ifndef LIT_AST_H
#define LIT_AST_H

#include "lit_lexer.h"

typedef enum {
	BINARY_AST
} LitExpresionType;

typedef struct {
	LitExpresionType type;
}	LitExpression;

typedef struct {
	LitExpression *expression;

	LitExpression* left;
	LitExpression* right;

	LitTokenType operator;
} LitBinaryExpression;

LitBinaryExpression* lit_make_binary_expression(LitVm* vm, LitExpression* left, LitExpression* right, LitTokenType operator);

#endif