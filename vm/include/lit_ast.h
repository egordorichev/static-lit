#ifndef LIT_AST_H
#define LIT_AST_H

#include "lit_lexer.h"

typedef enum {
	BINARY_EXPRESSION,
	LITERAL_EXPRESSION,
	UNARY_EXPRESSION,
	GROUPING_EXPRESSION,
	VAR_EXPRESSION,
	ASSIGN_EXPRESSION,
	LOGICAL_EXPRESSION
} LitExpresionType;

typedef struct {
	LitExpresionType type;
}	LitExpression;

typedef struct {
	int capacity;
	int count;
	LitExpression** values;
} LitExpressions;

void lit_init_expressions(LitExpressions* array);
void lit_free_expressions(LitVm* vm, LitExpressions* array);
void lit_expressions_write(LitVm* vm, LitExpressions* array, LitExpression* expression);

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

typedef struct {
	LitExpression* expression;
	LitToken* name;
	LitExpression* value;
} LitAssignExpression;

LitAssignExpression* lit_make_assign_expression(LitVm* vm, LitToken* name, LitExpression* value);

typedef struct {
	LitExpression* expression;
	LitTokenType operator;
	LitExpression* right;
} LitLogicalExpression;

LitLogicalExpression* lit_make_logical_expression(LitVm* vm, LitTokenType operator, LitExpression* right);

typedef enum {
	VAR_STATEMENT,
	EXPRESSION_STATEMENT,
	IF_STATEMENT,
	BLOCK_STATEMENT
} LitStatementType;

typedef struct {
	LitStatementType type;
}	LitStatement;

typedef struct {
	int capacity;
	int count;
	LitStatement** values;
} LitStatements;

void lit_init_statements(LitStatements* array);
void lit_free_statements(LitVm* vm, LitStatements* array);
void lit_statements_write(LitVm* vm, LitStatements* array, LitStatement* statement);

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

typedef struct {
	LitExpression* expression;
	LitExpression* condition;
	LitStatement* if_branch;
	LitStatement* else_branch;
	LitStatements* else_if_branches;
	LitExpressions* else_if_conditions;
} LitIfStatement;

LitIfStatement* lit_make_if_statement(LitVm* vm, LitExpression* condition, LitStatement* if_branch, LitStatement* else_branch, LitStatements* else_if_branches, LitExpressions* else_if_conditions);

typedef struct {
	LitExpression* expression;
	LitStatements* statements;
} LitBlockStatement;

LitBlockStatement* lit_make_block_statement(LitVm* vm, LitStatements* statements);

#endif