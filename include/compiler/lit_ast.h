#ifndef LIT_AST_H
#define LIT_AST_H

#include <lit_predefines.h>

#include <compiler/lit_lexer.h>
#include <util/lit_array.h>
#include <util/lit_table.h>

typedef struct LitParameter {
	const char* name;
	const char* type;
} LitParameter;

DECLARE_ARRAY(LitParameters, LitParameter, parameters)

typedef enum {
	BINARY_EXPRESSION = 0,
	LITERAL_EXPRESSION = 1,
	UNARY_EXPRESSION = 2,
	GROUPING_EXPRESSION = 3,
	VAR_EXPRESSION = 4,
	ASSIGN_EXPRESSION = 5,
	LOGICAL_EXPRESSION = 6,
	CALL_EXPRESSION = 7,
	LAMBDA_EXPRESSION = 8,
	GET_EXPRESSION = 9,
	SET_EXPRESSION = 10,
	THIS_EXPRESSION = 11,
	SUPER_EXPRESSION = 12
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

LitBinaryExpression* lit_make_binary_expression(LitCompiler* compiler, LitExpression* left, LitExpression* right, LitTokenType operator);

typedef struct {
	LitExpression *expression;
	LitValue value;
} LitLiteralExpression;

LitLiteralExpression* lit_make_literal_expression(LitCompiler* compiler, LitValue value);

typedef struct {
	LitExpression *expression;

	LitTokenType operator;
	LitExpression* right;
} LitUnaryExpression;

LitUnaryExpression* lit_make_unary_expression(LitCompiler* compiler, LitExpression* right, LitTokenType type);

typedef struct {
	LitExpression* expression;
	LitExpression* expr;
} LitGroupingExpression;

LitGroupingExpression* lit_make_grouping_expression(LitCompiler* compiler, LitExpression* expr);

typedef struct {
	LitExpression* expression;
	const char* name;
} LitVarExpression;

LitVarExpression* lit_make_var_expression(LitCompiler* compiler, const char* name);

typedef struct {
	LitExpression* expression;
	LitExpression* to;
	LitExpression* value;
} LitAssignExpression;

LitAssignExpression* lit_make_assign_expression(LitCompiler* compiler, LitExpression* to, LitExpression* value);

typedef struct {
	LitExpression* expression;
	LitTokenType operator;
	LitExpression* left;
	LitExpression* right;
} LitLogicalExpression;

LitLogicalExpression* lit_make_logical_expression(LitCompiler* compiler, LitTokenType operator, LitExpression* left, LitExpression* right);

typedef struct {
	LitExpression* expression;

	LitExpression* callee;
	LitExpressions* args;
} LitCallExpression;

LitCallExpression* lit_make_call_expression(LitCompiler* compiler, LitExpression* callee, LitExpressions* args);

typedef struct {
	LitExpression* expression;

	LitExpression* object;
	const char* property;
} LitGetExpression;

LitGetExpression* lit_make_get_expression(LitCompiler* compiler, LitExpression* object, const char* property);

typedef struct {
	LitExpression* expression;

	LitExpression* object;
	LitExpression* value;
	const char* property;
} LitSetExpression;

LitSetExpression* lit_make_set_expression(LitCompiler* compiler, LitExpression* object, LitExpression* value, const char* property);

typedef struct {
	LitExpression* expression;
} LitThisExpression;

LitThisExpression* lit_make_this_expression(LitCompiler* compiler);

typedef struct {
	LitExpression* expression;
	const char* method;
} LitSuperExpression;

LitSuperExpression* lit_make_super_expression(LitCompiler* compiler, const char* method);

typedef enum {
	VAR_STATEMENT,
	EXPRESSION_STATEMENT,
	IF_STATEMENT,
	BLOCK_STATEMENT,
	WHILE_STATEMENT,
	FUNCTION_STATEMENT,
	RETURN_STATEMENT,
	METHOD_STATEMENT,
	CLASS_STATEMENT,
	FIELD_STATEMENT,
	BREAK_STATEMENT,
	CONTINUE_STATEMENT
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

LitLambdaExpression* lit_make_lambda_expression(LitCompiler* compiler, LitParameters* parameters, LitStatement* body, LitParameter return_type);

typedef struct {
	LitStatement* expression;

	LitExpression* init;
	const char* name;
	const char* type;
	bool final;
} LitVarStatement;

LitVarStatement* lit_make_var_statement(LitCompiler* compiler, const char* name, LitExpression* init, const char* type, bool final);

typedef struct {
	LitStatement* expression;
	LitExpression* expr;
} LitExpressionStatement;

LitExpressionStatement* lit_make_expression_statement(LitCompiler* compiler, LitExpression* expr);

typedef struct {
	LitStatement* expression;

	LitExpression* condition;
	LitStatement* if_branch;
	LitStatement* else_branch;
	LitStatements* else_if_branches;
	LitExpressions* else_if_conditions;
} LitIfStatement;

LitIfStatement* lit_make_if_statement(LitCompiler* compiler, LitExpression* condition, LitStatement* if_branch, LitStatement* else_branch, LitStatements* else_if_branches, LitExpressions* else_if_conditions);

typedef struct {
	LitStatement* expression;
	LitStatements* statements;
} LitBlockStatement;

LitBlockStatement* lit_make_block_statement(LitCompiler* compiler, LitStatements* statements);

typedef struct {
	LitStatement* expression;

	LitExpression* condition;
	LitStatement* body;
} LitWhileStatement;

LitWhileStatement* lit_make_while_statement(LitCompiler* compiler, LitExpression* condition, LitStatement* body);

typedef struct {
	LitStatement* expression;

	LitParameters* parameters;
	LitStatement* body;
	LitParameter return_type;
	const char* name;
} LitFunctionStatement;

LitFunctionStatement* lit_make_function_statement(LitCompiler* compiler, const char* name, LitParameters* parameters, LitStatement* body, LitParameter return_type);
DECLARE_ARRAY(LitFunctions, LitFunctionStatement*, functions)

typedef struct {
	LitStatement* expression;
	LitExpression* value;
} LitReturnStatement;

LitReturnStatement* lit_make_return_statement(LitCompiler* compiler, LitExpression* value);

typedef struct LitField {
	LitValue value;
	const char* type;
	bool is_static;
} LitField;

DECLARE_TABLE(LitFields, LitField, fields, LitField*);

typedef enum LitAccessType {
	PUBLIC_ACCESS,
	PROTECTED_ACCESS,
	PRIVATE_ACCESS,
	UNDEFINED_ACCESS
} LitAccessType;

typedef struct {
	LitStatement* expression;

	LitExpression* init;
	LitStatement* getter;
	LitStatement* setter;
	LitAccessType access;
	bool is_static;
	bool final;
	const char* name;
	const char* type;
} LitFieldStatement;

LitFieldStatement* lit_make_field_statement(LitCompiler* compiler, const char* name, LitExpression* init, const char* type,
	LitStatement* getter, LitStatement* setter, LitAccessType access, bool is_static, bool final);

typedef struct {
	LitStatement* expression;

	LitParameters* parameters;
	LitStatement* body;
	LitParameter return_type;
	bool overriden;
	bool is_static;
	bool abstract;
	LitAccessType access;
	const char* name;
} LitMethodStatement;

LitMethodStatement* lit_make_method_statement(LitCompiler* compiler, const char* name, LitParameters* parameters, LitStatement* body, LitParameter return_type,
	bool overriden, bool is_static, bool abstract, LitAccessType access);

DECLARE_ARRAY(LitMethods, LitMethodStatement*, methods)

typedef struct {
	LitStatement* expression;

	LitVarExpression* super;
	LitMethods* methods;
	LitStatements* fields;
	bool abstract;
	bool is_static;
	bool final;
	const char* name;
} LitClassStatement;

LitClassStatement* lit_make_class_statement(LitCompiler* compiler, const char* name, LitVarExpression* super, LitMethods* methods, LitStatements* fields, bool abstract, bool is_static, bool final);

typedef struct {
	LitStatement* expression;
} LitBreakStatement;

LitBreakStatement* lit_make_break_statement(LitCompiler* compiler);

typedef struct {
	LitStatement* expression;
} LitContinueStatement;

LitContinueStatement* lit_make_continue_statement(LitCompiler* compiler);

void lit_free_statement(LitCompiler* compiler, LitStatement* statement);
void lit_free_expression(LitCompiler* compiler, LitExpression* expression);

#endif