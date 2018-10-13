#ifndef LIT_AST_H
#define LIT_AST_H

#include "lit_lexer.h"
#include "lit_array.h"

typedef struct LitParameter {
	const char* name;
	const char* type;
} LitParameter;

DECLARE_ARRAY(LitParameters, LitParameter, parameters)

typedef enum {
	BINARY_EXPRESSION,
	LITERAL_EXPRESSION,
	UNARY_EXPRESSION,
	GROUPING_EXPRESSION,
	VAR_EXPRESSION,
	ASSIGN_EXPRESSION,
	LOGICAL_EXPRESSION,
	CALL_EXPRESSION,
	LAMBDA_EXPRESSION
} LitExpresionType;

typedef struct LitExpression {
	LitExpresionType type;
} LitExpression;

DECLARE_ARRAY(LitExpressions, LitExpression*, expressions)

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
	LitExpression *expression;

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
	const char* name;
} LitVarExpression;

LitVarExpression* lit_make_var_expression(LitVm* vm, const char* name);

typedef struct {
	LitExpression* expression;
	const char* name;
	LitExpression* value;
} LitAssignExpression;

LitAssignExpression* lit_make_assign_expression(LitVm* vm, const char* name, LitExpression* value);

typedef struct {
	LitExpression* expression;
	LitTokenType operator;
	LitExpression* left;
	LitExpression* right;
} LitLogicalExpression;

LitLogicalExpression* lit_make_logical_expression(LitVm* vm, LitTokenType operator, LitExpression* left, LitExpression* right);

typedef struct {
	LitExpression* expression;

	LitExpression* callee;
	LitExpressions* args;
} LitCallExpression;

LitCallExpression* lit_make_call_expression(LitVm* vm, LitExpression* callee, LitExpressions* args);

typedef enum {
	VAR_STATEMENT,
	EXPRESSION_STATEMENT,
	IF_STATEMENT,
	BLOCK_STATEMENT,
	WHILE_STATEMENT,
	FUNCTION_STATEMENT,
	RETURN_STATEMENT,
	CLASS_STATEMENT
} LitStatementType;

typedef struct {
	LitStatementType type;
}	LitStatement;

DECLARE_ARRAY(LitStatements, LitStatement*, statements)

typedef struct {
	LitExpression* expression;

	LitParameters* parameters;
	LitStatement* body;
	LitParameter return_type;
} LitLambdaExpression;

LitLambdaExpression* lit_make_lambda_expression(LitVm* vm, LitParameters* parameters, LitStatement* body, LitParameter return_type);

typedef struct {
	LitStatement* expression;

	const char* name;
	LitExpression* init;
} LitVarStatement;

LitVarStatement* lit_make_var_statement(LitVm* vm, const char* name, LitExpression* init);

typedef struct {
	LitStatement* expression;
	LitExpression* expr;
} LitExpressionStatement;

LitExpressionStatement* lit_make_expression_statement(LitVm* vm, LitExpression* expr);

typedef struct {
	LitStatement* expression;

	LitExpression* condition;
	LitStatement* if_branch;
	LitStatement* else_branch;
	LitStatements* else_if_branches;
	LitExpressions* else_if_conditions;
} LitIfStatement;

LitIfStatement* lit_make_if_statement(LitVm* vm, LitExpression* condition, LitStatement* if_branch, LitStatement* else_branch, LitStatements* else_if_branches, LitExpressions* else_if_conditions);

typedef struct {
	LitStatement* expression;
	LitStatements* statements;
} LitBlockStatement;

LitBlockStatement* lit_make_block_statement(LitVm* vm, LitStatements* statements);

typedef struct {
	LitStatement* expression;

	LitExpression* condition;
	LitStatement* body;
} LitWhileStatement;

LitWhileStatement* lit_make_while_statement(LitVm* vm, LitExpression* condition, LitStatement* body);

typedef struct {
	LitStatement* expression;

	LitParameters* parameters;
	LitStatement* body;
	LitParameter return_type;
	const char* name;
} LitFunctionStatement;

LitFunctionStatement* lit_make_function_statement(LitVm* vm, const char* name, LitParameters* parameters, LitStatement* body, LitParameter return_type);
DECLARE_ARRAY(LitFunctions, LitFunctionStatement*, functions)

typedef struct {
	LitStatement* expression;
	LitExpression* value;
} LitReturnStatement;

LitReturnStatement* lit_make_return_statement(LitVm* vm, LitExpression* value);

typedef struct {
	LitStatement* expression;

	const char* name;
	LitVarExpression* super;
	LitFunctions* methods;
} LitClassStatement;

LitClassStatement* lit_make_class_statement(LitVm* vm, const char* name, LitVarExpression* super, LitFunctions* methods);

#endif