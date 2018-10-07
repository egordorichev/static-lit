#ifndef LIT_AST_H
#define LIT_AST_H

#include "lit_lexer.h"

typedef enum {
	BINARY_AST,
	LITERAL_AST,
	UNARY_AST,
	GROUPING_AST
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

typedef struct {
	LitExpression *expression;
	LitValue value;
} LitLiteralExpression;

LitLiteralExpression* lit_make_literal_expression(LitVm* vm, LitValue value);

typedef struct {
	LitTokenType operator;
	LitExpression* right;
} LitUnaryExpression;

LitUnaryExpression* lit_make_unary_expression(LitVm* vm, LitExpression* right, LitTokenType type);

typedef struct {
		LitExpression *expression;
		LitExpression *expr;
} LitGroupingExpression;

LitGroupingExpression* lit_make_grouping_expression(LitVm* vm, LitExpression* expr);

#endif