#ifndef LIT_AST_H
#define LIT_AST_H

#include "lit_lexer.h"

typedef enum {
	BINARY_EXPRESSION,
	LITERAL_EXPRESSION,
	UNARY_EXPRESSION,
	GROUPING_EXPRESSION,
	VAR_EXPRESSION
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
	LitExpression* expression;
	LitExpression* expr;
} LitGroupingExpression;

LitGroupingExpression* lit_make_grouping_expression(LitVm* vm, LitExpression* expr);

typedef struct {
	LitExpression* expression;
	LitToken* name;
} LitVarExpression;

LitVarExpression* lit_make_var_expression(LitVm* vm, LitToken* name);

typedef enum {
	VAR_STATEMENT,
	EXPRESSION_STATEMENT
} LitStatementType;

typedef struct {
	LitStatementType type;
}	LitStatement;

typedef struct {
	LitExpression* expression;

	LitToken* name;
	LitExpression* init;
} LitVarStatement;

LitVarStatement* lit_make_var_statement(LitVm* vm, LitToken* name, LitExpression* init);

typedef struct {
	LitExpression* expression;
	LitExpression* expr;
} LitExpressionStatement;

LitExpressionStatement* lit_make_expression_statement(LitVm* vm, LitExpression* expr);

#endif