#include <stdio.h>
#include <memory.h>
#include <lit.h>

#include "lit_ast.h"
#include "lit_memory.h"
#include "lit_array.h"
#include "lit_object.h"

DEFINE_ARRAY(LitParameters, LitParameter, parameters)
DEFINE_ARRAY(LitExpressions, LitExpression*, expressions)
DEFINE_ARRAY(LitStatements, LitStatement*, statements)
DEFINE_ARRAY(LitFunctions, LitFunctionStatement*, functions)
DEFINE_TABLE(LitFields, LitField, fields, (LitField) {});

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

LitVarExpression* lit_make_var_expression(LitVm* vm, const char* name) {
	LitVarExpression* expression = ALLOCATE_EXPRESSION(vm, LitVarExpression, VAR_EXPRESSION);

	expression->name = name;

	return expression;
}

LitAssignExpression* lit_make_assign_expression(LitVm* vm, const char* name, LitExpression* value) {
	LitAssignExpression* expression = ALLOCATE_EXPRESSION(vm, LitAssignExpression, ASSIGN_EXPRESSION);

	expression->name = name;
	expression->value = value;

	return expression;
}

LitLogicalExpression* lit_make_logical_expression(LitVm* vm, LitTokenType operator, LitExpression* left, LitExpression* right) {
	LitLogicalExpression* expression = ALLOCATE_EXPRESSION(vm, LitLogicalExpression, LOGICAL_EXPRESSION);

	expression->operator = operator;
	expression->left = left;
	expression->right = right;

	return expression;
}

LitCallExpression* lit_make_call_expression(LitVm* vm, LitExpression* callee, LitExpressions* args) {
	LitCallExpression* expression = ALLOCATE_EXPRESSION(vm, LitCallExpression, CALL_EXPRESSION);

	expression->callee = callee;
	expression->args = args;

	return expression;
}

LitGetExpression* lit_make_get_expression(LitVm* vm, LitExpression* object, const char* property) {
	LitGetExpression* expression = ALLOCATE_EXPRESSION(vm, LitGetExpression, GET_EXPRESSION);

	expression->object = object;
	expression->property = property;

	return expression;
}

LitSetExpression* lit_make_set_expression(LitVm* vm, LitExpression* object, LitExpression* value, const char* property) {
	LitSetExpression* expression = ALLOCATE_EXPRESSION(vm, LitSetExpression, SET_EXPRESSION);

	expression->object = object;
	expression->value = value;
	expression->property = property;

	return expression;
}

LitThisExpression* lit_make_this_expression(LitVm* vm) {
	return ALLOCATE_EXPRESSION(vm, LitThisExpression, THIS_EXPRESSION);
}

LitSuperExpression* lit_make_super_expression(LitVm* vm, const char* method) {
	LitSuperExpression* expression = ALLOCATE_EXPRESSION(vm, LitSuperExpression, SUPER_EXPRESSION);

	expression->method = method;

	return expression;
}

LitLambdaExpression* lit_make_lambda_expression(LitVm* vm, LitParameters* parameters, LitStatement* body, LitParameter return_type) {
	LitLambdaExpression* expression = ALLOCATE_EXPRESSION(vm, LitLambdaExpression, LAMBDA_EXPRESSION);

	expression->parameters = parameters;
	expression->body = body;
	expression->return_type = return_type;

	return expression;
}

LitVarStatement* lit_make_var_statement(LitVm* vm, const char* name, LitExpression* init) {
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

LitIfStatement* lit_make_if_statement(LitVm* vm, LitExpression* condition, LitStatement* if_branch, LitStatement* else_branch, LitStatements* else_if_branches, LitExpressions* else_if_conditions) {
	LitIfStatement* statement = ALLOCATE_STATEMENT(vm, LitIfStatement, IF_STATEMENT);

	statement->condition = condition;
	statement->if_branch = if_branch;
	statement->else_branch = else_branch;
	statement->else_if_branches = else_if_branches;
	statement->else_if_conditions = else_if_conditions;

	return statement;
}

LitBlockStatement* lit_make_block_statement(LitVm* vm, LitStatements* statements) {
	LitBlockStatement* statement = ALLOCATE_STATEMENT(vm, LitBlockStatement, BLOCK_STATEMENT);

	statement->statements = statements;

	return statement;
}

LitWhileStatement* lit_make_while_statement(LitVm* vm, LitExpression* condition, LitStatement* body) {
	LitWhileStatement* statement = ALLOCATE_STATEMENT(vm, LitWhileStatement, WHILE_STATEMENT);

	statement->condition = condition;
	statement->body = body;

	return statement;
}

LitFunctionStatement* lit_make_function_statement(LitVm* vm, const char* name, LitParameters* parameters, LitStatement* body, LitParameter return_type) {
	LitFunctionStatement* statement = ALLOCATE_STATEMENT(vm, LitFunctionStatement, FUNCTION_STATEMENT);

	statement->name = name;
	statement->parameters = parameters;
	statement->body = body;
	statement->return_type = return_type;

	return statement;
}

LitReturnStatement* lit_make_return_statement(LitVm* vm, LitExpression* value) {
	LitReturnStatement* statement = ALLOCATE_STATEMENT(vm, LitReturnStatement, RETURN_STATEMENT);

	statement->value = value;

	return statement;
}

LitClassStatement* lit_make_class_statement(LitVm* vm, const char* name, LitVarExpression* super, LitFunctions* methods, LitStatements* fields) {
	LitClassStatement* statement = ALLOCATE_STATEMENT(vm, LitClassStatement, CLASS_STATEMENT);

	statement->name = name;
	statement->super = super;
	statement->methods = methods;
	statement->fields = fields;

	return statement;
}

void lit_free_statement(LitVm* vm, LitStatement* statement) {
	switch (statement->type) {
		case VAR_STATEMENT: {
			LitVarStatement* stmt = (LitVarStatement*) statement;
			reallocate(vm, (void*) stmt->name, strlen(stmt->name) + 1, 0);

			if (stmt->init != NULL) {
				lit_free_expression(vm, stmt->init);
			}

			reallocate(vm, (void*) statement, sizeof(LitVarStatement), 0);
			break;
		}
		case EXPRESSION_STATEMENT: {
			lit_free_expression(vm, ((LitExpressionStatement*) statement)->expr);
			reallocate(vm, (void*) statement, sizeof(LitExpressionStatement), 0);

			break;
		}
		case IF_STATEMENT: {
			LitIfStatement* stmt = (LitIfStatement*) statement;

			lit_free_expression(vm, stmt->condition);
			lit_free_statement(vm, stmt->if_branch);

			if (stmt->else_if_branches != NULL) {
				for (int i = 0; i < stmt->else_if_branches->count; i++) {
					lit_free_expression(vm, stmt->else_if_conditions->values[i]);
					lit_free_statement(vm, stmt->else_if_branches->values[i]);
				}

				lit_free_expressions(vm, stmt->else_if_conditions);
				lit_free_statements(vm, stmt->else_if_branches);

				reallocate(vm, (void*) stmt->else_if_conditions, sizeof(LitExpressions), 0);
				reallocate(vm, (void*) stmt->else_if_branches, sizeof(LitStatements), 0);
			}

			if (stmt->else_branch != NULL) {
				lit_free_statement(vm, stmt->else_branch);
			}

			reallocate(vm, (void*) statement, sizeof(LitIfStatement), 0);
			break;
		}
		case BLOCK_STATEMENT: {
			LitBlockStatement* stmt = (LitBlockStatement*) statement;

			if (stmt->statements != NULL) {
				for (int i = 0; i < stmt->statements->count; i++) {
					lit_free_statement(vm, stmt->statements->values[i]);
				}

				lit_free_statements(vm, stmt->statements);
				reallocate(vm, (void*) stmt->statements, sizeof(LitStatements), 0);
			}

			reallocate(vm, (void*) statement, sizeof(LitBlockStatement), 0);

			break;
		}
		case WHILE_STATEMENT: {
			LitWhileStatement* stmt = (LitWhileStatement*) statement;

			lit_free_expression(vm, stmt->condition);
			lit_free_statement(vm, stmt->body);
			reallocate(vm, (void*) statement, sizeof(LitWhileStatement), 0);

			break;
		}
		case FUNCTION_STATEMENT: {
			LitFunctionStatement* stmt = (LitFunctionStatement*) statement;

			reallocate(vm, (void*) stmt->name, strlen(stmt->name) + 1, 0);

			if (strcmp(stmt->return_type.type, "void") != 0) {
				reallocate(vm, (void*) stmt->return_type.type, strlen(stmt->return_type.type) + 1, 0);
			}

			lit_free_statement(vm, stmt->body);

			if (stmt->parameters != NULL) {
				for (int i = 0; i  < stmt->parameters->count; i++) {
					LitParameter parameter = stmt->parameters->values[i];

					reallocate(vm, (void*) parameter.name, strlen(parameter.name) + 1, 0);
					reallocate(vm, (void*) parameter.type, strlen(parameter.type) + 1, 0);
				}

				lit_free_parameters(vm, stmt->parameters);
			}

			reallocate(vm, (void*) statement, sizeof(LitFunctionStatement), 0);
			break;
		}
		case RETURN_STATEMENT: {
			LitReturnStatement* stmt = (LitReturnStatement*) statement;

			if (stmt->value != NULL) {
				lit_free_expression(vm, stmt->value);
			}

			reallocate(vm, (void*) statement, sizeof(LitReturnStatement), 0);
			break;
		}
		case CLASS_STATEMENT: {
			LitClassStatement* stmt = (LitClassStatement*) statement;
			reallocate(vm, (void*) stmt->name, strlen(stmt->name) + 1, 0);

			if (stmt->fields != NULL) {
				for (int i = 0; i < stmt->fields->count; i++) {
					lit_free_statement(vm, stmt->fields->values[i]);
				}

				lit_free_statements(vm, stmt->fields);
				reallocate(vm, (void*) stmt->fields, sizeof(LitStatements), 0);
			}

			if (stmt->methods != NULL) {
				for (int i = 0; i < stmt->methods->count; i++) {
					lit_free_statement(vm, (LitStatement*) stmt->methods->values[i]);
				}

				lit_free_functions(vm, stmt->methods);
				reallocate(vm, (void*) stmt->methods, sizeof(LitFunctions), 0);
			}

			if (stmt->super != NULL) {
				lit_free_expression(vm, (LitExpression*) stmt->super);
			}

			reallocate(vm, (void*) statement, sizeof(LitClassStatement), 0);
			break;
		}
	}
}

void lit_free_expression(LitVm* vm, LitExpression* expression) {
	switch (expression->type) {
		case BINARY_EXPRESSION: {
			LitBinaryExpression* expr = (LitBinaryExpression*) expression;

			lit_free_expression(vm, expr->left);
			lit_free_expression(vm, expr->right);
			reallocate(vm, (void*) expression, sizeof(LitBinaryExpression), 0);

			break;
		}
		case LITERAL_EXPRESSION: {
			reallocate(vm, (void*) expression, sizeof(LitLiteralExpression), 0);
			break;
		}
		case UNARY_EXPRESSION: {
			LitUnaryExpression* expr = (LitUnaryExpression*) expression;

			lit_free_expression(vm, expr->right);
			reallocate(vm, (void*) expression, sizeof(LitUnaryExpression), 0);

			break;
		}
		case GROUPING_EXPRESSION: {
			LitGroupingExpression* expr = (LitGroupingExpression*) expression;

			lit_free_expression(vm, expr->expr);
			reallocate(vm, (void*) expression, sizeof(LitGroupingExpression), 0);

			break;
		}
		case VAR_EXPRESSION: {
			LitVarExpression* expr = (LitVarExpression*) expression;

			reallocate(vm, (void*) expr->name, strlen(expr->name) + 1, 0);
			reallocate(vm, (void*) expression, sizeof(LitVarExpression), 0);

			break;
		}
		case ASSIGN_EXPRESSION: {
			// FIXME: leak from 173 to 189
			LitAssignExpression* expr = (LitAssignExpression*) expression;

			lit_free_expression(vm, expr->value);
			reallocate(vm, (void*) expression, sizeof(LitAssignExpression), 0);
			reallocate(vm, (void*) expr->name, strlen(expr->name) + 1, 0);

			break;
		}
		case LOGICAL_EXPRESSION: {
			LitLogicalExpression* expr = (LitLogicalExpression*) expression;

			lit_free_expression(vm, expr->left);
			lit_free_expression(vm, expr->right);
			reallocate(vm, (void*) expression, sizeof(LitLogicalExpression), 0);

			break;
		}
		case CALL_EXPRESSION: {
			LitCallExpression* expr = (LitCallExpression*) expression;
			lit_free_expression(vm, expr->callee);

			for (int i = 0; i < expr->args->count; i++) {
				lit_free_expression(vm, expr->args->values[i]);
			}

			lit_free_expressions(vm, expr->args);
			reallocate(vm, (void*) expr->args, sizeof(LitExpressions), 0);

			reallocate(vm, (void*) expression, sizeof(LitCallExpression), 0);

			break;
		}
		case LAMBDA_EXPRESSION: {
			LitLambdaExpression* expr = (LitLambdaExpression*) expression;

			if (strcmp(expr->return_type.type, "void") != 0) {
				reallocate(vm, (void*) expr->return_type.type, strlen(expr->return_type.type) + 1, 0);
			}

			lit_free_statement(vm, expr->body);

			if (expr->parameters != NULL) {
				for (int i = 0; i  < expr->parameters->count; i++) {
					LitParameter parameter = expr->parameters->values[i];

					reallocate(vm, (void*) parameter.name, strlen(parameter.name) + 1, 0);
					reallocate(vm, (void*) parameter.type, strlen(parameter.type) + 1, 0);
				}

				lit_free_parameters(vm, expr->parameters);
			}

			reallocate(vm, (void*) expression, sizeof(LitLambdaExpression), 0);
			break;
		}
		case GET_EXPRESSION: {
			LitGetExpression* expr = (LitGetExpression*) expression;

			lit_free_expression(vm, expr->object);
			reallocate(vm, (void*) expr->property, strlen(expr->property) + 1, 0);
			reallocate(vm, (void*) expression, sizeof(LitGetExpression), 0);

			break;
		}
		case SET_EXPRESSION: {
			LitSetExpression* expr = (LitSetExpression*) expression;

			lit_free_expression(vm, expr->object);
			lit_free_expression(vm, expr->value);
			reallocate(vm, (void*) expr->property, strlen(expr->property) + 1, 0);
			reallocate(vm, (void*) expression, sizeof(LitSetExpression), 0);

			break;
		}
		case THIS_EXPRESSION: {
			reallocate(vm, (void*) expression, sizeof(LitThisExpression), 0);

			break;
		}
		case SUPER_EXPRESSION: {
			// FIXME: leak from 173 to 189
			LitSuperExpression* expr = (LitSuperExpression*) expression;

			reallocate(vm, (void*) expr->method, strlen(expr->method) + 1, 0);
			reallocate(vm, (void*) expression, sizeof(LitSuperExpression), 0);

			break;
		}
	}
}