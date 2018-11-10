#include <stdio.h>
#include <memory.h>
#include <lit.h>

#include <compiler/lit_ast.h>
#include <vm/lit_memory.h>
#include <vm/lit_object.h>
#include <util/lit_array.h>

DEFINE_ARRAY(LitParameters, LitParameter, parameters)
DEFINE_ARRAY(LitExpressions, LitExpression*, expressions)
DEFINE_ARRAY(LitStatements, LitStatement*, statements)
DEFINE_ARRAY(LitFunctions, LitFunctionStatement*, functions)
DEFINE_ARRAY(LitMethods, LitMethodStatement*, methods)
DEFINE_TABLE(LitFields, LitField, fields, LitField*, (LitField) {}, &entry->value);

#define ALLOCATE_EXPRESSION(compiler, type, object_type) \
    (type*) allocate_expression(compiler, sizeof(type), object_type)

static LitExpression* allocate_expression(LitCompiler* compiler, size_t size, LitExpresionType type) {
	LitExpression* object = (LitExpression*) reallocate(compiler, NULL, 0, size);
	object->type = type;
	return object;
}

#define ALLOCATE_STATEMENT(compiler, type, object_type) \
    (type*) allocate_statement(compiler, sizeof(type), object_type)

static LitStatement* allocate_statement(LitCompiler* compiler, size_t size, LitStatementType type) {
	LitStatement* object = (LitStatement*) reallocate(compiler, NULL, 0, size);
	object->type = type;
	return object;
}

LitBinaryExpression* lit_make_binary_expression(LitCompiler* compiler, LitExpression* left, LitExpression* right, LitTokenType operator) {
	LitBinaryExpression* expression = ALLOCATE_EXPRESSION(compiler, LitBinaryExpression, BINARY_EXPRESSION);

	expression->left = left;
	expression->right = right;
	expression->ignore_left = false;
	expression->operator = operator;

	return expression;
}

LitLiteralExpression* lit_make_literal_expression(LitCompiler* compiler, LitValue value) {
	LitLiteralExpression* expression = ALLOCATE_EXPRESSION(compiler, LitLiteralExpression, LITERAL_EXPRESSION);

	expression->value = value;

	return expression;
}

LitUnaryExpression* lit_make_unary_expression(LitCompiler* compiler, LitExpression* right, LitTokenType type) {
	LitUnaryExpression* expression = ALLOCATE_EXPRESSION(compiler, LitUnaryExpression, UNARY_EXPRESSION);

	expression->right = right;
	expression->operator = type;

	return expression;
}

LitGroupingExpression* lit_make_grouping_expression(LitCompiler* compiler, LitExpression* expr) {
	LitGroupingExpression* expression = ALLOCATE_EXPRESSION(compiler, LitGroupingExpression, GROUPING_EXPRESSION);

	expression->expr = expr;

	return expression;
}

LitVarExpression* lit_make_var_expression(LitCompiler* compiler, const char* name) {
	LitVarExpression* expression = ALLOCATE_EXPRESSION(compiler, LitVarExpression, VAR_EXPRESSION);

	expression->name = name;

	return expression;
}

LitAssignExpression* lit_make_assign_expression(LitCompiler* compiler, LitExpression* to, LitExpression* value) {
	LitAssignExpression* expression = ALLOCATE_EXPRESSION(compiler, LitAssignExpression, ASSIGN_EXPRESSION);

	expression->to = to;
	expression->value = value;

	return expression;
}

LitLogicalExpression* lit_make_logical_expression(LitCompiler* compiler, LitTokenType operator, LitExpression* left, LitExpression* right) {
	LitLogicalExpression* expression = ALLOCATE_EXPRESSION(compiler, LitLogicalExpression, LOGICAL_EXPRESSION);

	expression->operator = operator;
	expression->left = left;
	expression->right = right;

	return expression;
}

LitCallExpression* lit_make_call_expression(LitCompiler* compiler, LitExpression* callee, LitExpressions* args) {
	LitCallExpression* expression = ALLOCATE_EXPRESSION(compiler, LitCallExpression, CALL_EXPRESSION);

	expression->callee = callee;
	expression->args = args;

	return expression;
}

LitGetExpression* lit_make_get_expression(LitCompiler* compiler, LitExpression* object, const char* property) {
	LitGetExpression* expression = ALLOCATE_EXPRESSION(compiler, LitGetExpression, GET_EXPRESSION);

	expression->object = object;
	expression->property = property;
	expression->emit_static_init = false;

	return expression;
}

LitSetExpression* lit_make_set_expression(LitCompiler* compiler, LitExpression* object, LitExpression* value, const char* property) {
	LitSetExpression* expression = ALLOCATE_EXPRESSION(compiler, LitSetExpression, SET_EXPRESSION);

	expression->object = object;
	expression->value = value;
	expression->property = property;
	expression->emit_static_init = false;

	return expression;
}

LitThisExpression* lit_make_this_expression(LitCompiler* compiler) {
	return ALLOCATE_EXPRESSION(compiler, LitThisExpression, THIS_EXPRESSION);
}

LitSuperExpression* lit_make_super_expression(LitCompiler* compiler, const char* method) {
	LitSuperExpression* expression = ALLOCATE_EXPRESSION(compiler, LitSuperExpression, SUPER_EXPRESSION);

	expression->method = method;

	return expression;
}

LitIfExpression* lit_make_if_expression(LitCompiler* compiler, LitExpression* condition, LitExpression* if_branch, LitExpression* else_branch, LitExpressions* else_if_branches, LitExpressions* else_if_conditions) {
	LitIfExpression* expression = ALLOCATE_EXPRESSION(compiler, LitIfExpression, IF_EXPRESSION);

	expression->condition = condition;
	expression->if_branch = if_branch;
	expression->else_branch = else_branch;
	expression->else_if_branches = else_if_branches;
	expression->else_if_conditions = else_if_conditions;

	return expression;
}

LitLambdaExpression* lit_make_lambda_expression(LitCompiler* compiler, LitParameters* parameters, LitStatement* body, LitParameter return_type) {
	LitLambdaExpression* expression = ALLOCATE_EXPRESSION(compiler, LitLambdaExpression, LAMBDA_EXPRESSION);

	expression->parameters = parameters;
	expression->body = body;
	expression->return_type = return_type;

	return expression;
}

LitVarStatement* lit_make_var_statement(LitCompiler* compiler, const char* name, LitExpression* init, const char* type, bool final) {
	LitVarStatement* statement = ALLOCATE_STATEMENT(compiler, LitVarStatement, VAR_STATEMENT);

	statement->name = name;
	statement->init = init;
	statement->type = type;
	statement->final = final;

	return statement;
}

LitExpressionStatement* lit_make_expression_statement(LitCompiler* compiler, LitExpression* expr) {
	LitExpressionStatement* statement = ALLOCATE_STATEMENT(compiler, LitExpressionStatement, EXPRESSION_STATEMENT);

	statement->expr = expr;

	return statement;
}

LitIfStatement* lit_make_if_statement(LitCompiler* compiler, LitExpression* condition, LitStatement* if_branch, LitStatement* else_branch, LitStatements* else_if_branches, LitExpressions* else_if_conditions) {
	LitIfStatement* statement = ALLOCATE_STATEMENT(compiler, LitIfStatement, IF_STATEMENT);

	statement->condition = condition;
	statement->if_branch = if_branch;
	statement->else_branch = else_branch;
	statement->else_if_branches = else_if_branches;
	statement->else_if_conditions = else_if_conditions;

	return statement;
}

LitBlockStatement* lit_make_block_statement(LitCompiler* compiler, LitStatements* statements) {
	LitBlockStatement* statement = ALLOCATE_STATEMENT(compiler, LitBlockStatement, BLOCK_STATEMENT);

	statement->statements = statements;

	return statement;
}

LitWhileStatement* lit_make_while_statement(LitCompiler* compiler, LitExpression* condition, LitStatement* body) {
	LitWhileStatement* statement = ALLOCATE_STATEMENT(compiler, LitWhileStatement, WHILE_STATEMENT);

	statement->condition = condition;
	statement->body = body;

	return statement;
}

LitFunctionStatement* lit_make_function_statement(LitCompiler* compiler, const char* name, LitParameters* parameters, LitStatement* body, LitParameter return_type) {
	LitFunctionStatement* statement = ALLOCATE_STATEMENT(compiler, LitFunctionStatement, FUNCTION_STATEMENT);

	statement->name = name;
	statement->parameters = parameters;
	statement->body = body;
	statement->return_type = return_type;

	return statement;
}

LitReturnStatement* lit_make_return_statement(LitCompiler* compiler, LitExpression* value) {
	LitReturnStatement* statement = ALLOCATE_STATEMENT(compiler, LitReturnStatement, RETURN_STATEMENT);

	statement->value = value;

	return statement;
}

LitFieldStatement* lit_make_field_statement(LitCompiler* compiler, const char* name, LitExpression* init, const char* type,
	LitStatement* getter, LitStatement* setter, LitAccessType access, bool is_static, bool final) {

	LitFieldStatement* statement = ALLOCATE_STATEMENT(compiler, LitFieldStatement, FIELD_STATEMENT);

	statement->name = name;
	statement->init = init;
	statement->type = type;
	statement->getter = getter;
	statement->setter = setter;
	statement->access = access;
	statement->is_static = is_static;
	statement->final = final;

	return statement;
}

LitMethodStatement* lit_make_method_statement(LitCompiler* compiler, const char* name, LitParameters* parameters, LitStatement* body,
	LitParameter return_type, bool overriden, bool is_static, bool abstract, LitAccessType access) {

	LitMethodStatement* statement = ALLOCATE_STATEMENT(compiler, LitMethodStatement, METHOD_STATEMENT);

	statement->name = name;
	statement->parameters = parameters;
	statement->body = body;
	statement->return_type = return_type;
	statement->overriden = overriden;
	statement->is_static = is_static;
	statement->abstract = abstract;
	statement->access = access;

	return statement;
}

LitClassStatement* lit_make_class_statement(LitCompiler* compiler, const char* name, LitVarExpression* super, LitMethods* methods,
		LitStatements* fields, bool abstract, bool is_static, bool final) {

	LitClassStatement* statement = ALLOCATE_STATEMENT(compiler, LitClassStatement, CLASS_STATEMENT);

	statement->name = name;
	statement->super = super;
	statement->methods = methods;
	statement->fields = fields;
	statement->abstract = abstract;
	statement->is_static = is_static;
	statement->final = final;

	return statement;
}

LitBreakStatement* lit_make_break_statement(LitCompiler* compiler) {
	return ALLOCATE_STATEMENT(compiler, LitBreakStatement, BREAK_STATEMENT);
}

LitContinueStatement* lit_make_continue_statement(LitCompiler* compiler) {
	return ALLOCATE_STATEMENT(compiler, LitContinueStatement, CONTINUE_STATEMENT);
}

void lit_free_statement(LitCompiler* compiler, LitStatement* statement) {
	switch (statement->type) {
		case VAR_STATEMENT: {
			LitVarStatement* stmt = (LitVarStatement*) statement;
			reallocate(compiler, (void*) stmt->name, strlen(stmt->name) + 1, 0);

			if (stmt->type != NULL) {
				reallocate(compiler, (void*) stmt->type, strlen(stmt->type) + 1, 0);
			}

			if (stmt->init != NULL) {
				lit_free_expression(compiler, stmt->init);
			}

			reallocate(compiler, (void*) statement, sizeof(LitVarStatement), 0);
			break;
		}
		case FIELD_STATEMENT: {
			LitFieldStatement* stmt = (LitFieldStatement*) statement;
			reallocate(compiler, (void*) stmt->name, strlen(stmt->name) + 1, 0);

			/*if (stmt->type != NULL) {
				reallocate(compiler, (void*) stmt->type, strlen(stmt->type) + 1, 0);
			}*/ // Not sure if thats needed, crashes here, FIXME

			if (stmt->init != NULL) {
				lit_free_expression(compiler, stmt->init);
			}

			if (stmt->getter != NULL) {
				lit_free_statement(compiler, stmt->getter);
			}

			if (stmt->setter != NULL) {
				lit_free_statement(compiler, stmt->setter);
			}

			reallocate(compiler, (void*) statement, sizeof(LitFieldStatement), 0);
			break;
		}
		case EXPRESSION_STATEMENT: {
			lit_free_expression(compiler, ((LitExpressionStatement*) statement)->expr);
			reallocate(compiler, (void*) statement, sizeof(LitExpressionStatement), 0);

			break;
		}
		case IF_STATEMENT: {
			LitIfStatement* stmt = (LitIfStatement*) statement;

			lit_free_expression(compiler, stmt->condition);
			lit_free_statement(compiler, stmt->if_branch);

			if (stmt->else_if_branches != NULL) {
				for (int i = 0; i < stmt->else_if_branches->count; i++) {
					lit_free_expression(compiler, stmt->else_if_conditions->values[i]);
					lit_free_statement(compiler, stmt->else_if_branches->values[i]);
				}

				lit_free_expressions(compiler, stmt->else_if_conditions);
				lit_free_statements(compiler, stmt->else_if_branches);

				reallocate(compiler, (void*) stmt->else_if_conditions, sizeof(LitExpressions), 0);
				reallocate(compiler, (void*) stmt->else_if_branches, sizeof(LitStatements), 0);
			}

			if (stmt->else_branch != NULL) {
				lit_free_statement(compiler, stmt->else_branch);
			}

			reallocate(compiler, (void*) statement, sizeof(LitIfStatement), 0);
			break;
		}
		case BLOCK_STATEMENT: {
			LitBlockStatement* stmt = (LitBlockStatement*) statement;

			if (stmt->statements != NULL) {
				for (int i = 0; i < stmt->statements->count; i++) {
					lit_free_statement(compiler, stmt->statements->values[i]);
				}

				lit_free_statements(compiler, stmt->statements);
				reallocate(compiler, (void*) stmt->statements, sizeof(LitStatements), 0);
			}

			reallocate(compiler, (void*) statement, sizeof(LitBlockStatement), 0);

			break;
		}
		case WHILE_STATEMENT: {
			LitWhileStatement* stmt = (LitWhileStatement*) statement;

			lit_free_expression(compiler, stmt->condition);
			lit_free_statement(compiler, stmt->body);
			reallocate(compiler, (void*) statement, sizeof(LitWhileStatement), 0);

			break;
		}
		case FUNCTION_STATEMENT: {
			LitFunctionStatement* stmt = (LitFunctionStatement*) statement;
			reallocate(compiler, (void*) stmt->name, strlen(stmt->name) + 1, 0);

			if (strcmp(stmt->return_type.type, "void") != 0) {
				reallocate(compiler, (void*) stmt->return_type.type, strlen(stmt->return_type.type) + 1, 0);
			}

			lit_free_statement(compiler, stmt->body);

			if (stmt->parameters != NULL) {
				for (int i = 0; i  < stmt->parameters->count; i++) {
					LitParameter parameter = stmt->parameters->values[i];

					reallocate(compiler, (void*) parameter.name, strlen(parameter.name) + 1, 0);
					reallocate(compiler, (void*) parameter.type, strlen(parameter.type) + 1, 0);
				}

				lit_free_parameters(compiler, stmt->parameters);
				reallocate(compiler, (void*) stmt->parameters, sizeof(LitParameters), 0);
			}

			reallocate(compiler, (void*) statement, sizeof(LitFunctionStatement), 0);
			break;
		}
		case METHOD_STATEMENT: {
			LitMethodStatement* stmt = (LitMethodStatement*) statement;
			reallocate(compiler, (void*) stmt->name, strlen(stmt->name) + 1, 0);

			if (strcmp(stmt->return_type.type, "void") != 0) {
				reallocate(compiler, (void*) stmt->return_type.type, strlen(stmt->return_type.type) + 1, 0);
			}

			lit_free_statement(compiler, stmt->body);

			if (stmt->parameters != NULL) {
				for (int i = 0; i  < stmt->parameters->count; i++) {
					LitParameter parameter = stmt->parameters->values[i];

					reallocate(compiler, (void*) parameter.name, strlen(parameter.name) + 1, 0);
					reallocate(compiler, (void*) parameter.type, strlen(parameter.type) + 1, 0);
				}

				lit_free_parameters(compiler, stmt->parameters);
				reallocate(compiler, (void*) stmt->parameters, sizeof(LitParameters), 0);
			}

			reallocate(compiler, (void*) statement, sizeof(LitMethodStatement), 0);
			break;
		}
		case RETURN_STATEMENT: {
			LitReturnStatement* stmt = (LitReturnStatement*) statement;

			if (stmt->value != NULL) {
				lit_free_expression(compiler, stmt->value);
			}

			reallocate(compiler, (void*) statement, sizeof(LitReturnStatement), 0);
			break;
		}
		case CLASS_STATEMENT: {
			LitClassStatement* stmt = (LitClassStatement*) statement;
			reallocate(compiler, (void*) stmt->name, strlen(stmt->name) + 1, 0);

			if (stmt->fields != NULL) {
				for (int i = 0; i < stmt->fields->count; i++) {
					lit_free_statement(compiler, stmt->fields->values[i]);
				}

				lit_free_statements(compiler, stmt->fields);
				reallocate(compiler, (void*) stmt->fields, sizeof(LitStatements), 0);
			}

			if (stmt->methods != NULL) {
				for (int i = 0; i < stmt->methods->count; i++) {
					lit_free_statement(compiler, (LitStatement*) stmt->methods->values[i]);
				}

				lit_free_methods(compiler, stmt->methods);
				reallocate(compiler, (void*) stmt->methods, sizeof(LitFunctions), 0);
			}

			if (stmt->super != NULL) {
				lit_free_expression(compiler, (LitExpression*) stmt->super);
			}

			reallocate(compiler, (void*) statement, sizeof(LitClassStatement), 0);
			break;
		}
		case BREAK_STATEMENT:
		case CONTINUE_STATEMENT: {
			break;
		}
		default: {
			fflush(stdin);
			fprintf(stderr, "Statement with id %i has no freeing case!\n", statement->type);
			fflush(stdout);

			break;
		}
	}
}

void lit_free_expression(LitCompiler* compiler, LitExpression* expression) {
	switch (expression->type) {
		case BINARY_EXPRESSION: {
			LitBinaryExpression* expr = (LitBinaryExpression*) expression;

			if (!expr->ignore_left) {
				lit_free_expression(compiler, expr->left);
			}

			lit_free_expression(compiler, expr->right);
			reallocate(compiler, (void*) expression, sizeof(LitBinaryExpression), 0);

			break;
		}
		case LITERAL_EXPRESSION: {
			reallocate(compiler, (void*) expression, sizeof(LitLiteralExpression), 0);
			break;
		}
		case UNARY_EXPRESSION: {
			LitUnaryExpression* expr = (LitUnaryExpression*) expression;

			lit_free_expression(compiler, expr->right);
			reallocate(compiler, (void*) expression, sizeof(LitUnaryExpression), 0);

			break;
		}
		case GROUPING_EXPRESSION: {
			LitGroupingExpression* expr = (LitGroupingExpression*) expression;

			lit_free_expression(compiler, expr->expr);
			reallocate(compiler, (void*) expression, sizeof(LitGroupingExpression), 0);

			break;
		}
		case VAR_EXPRESSION: {
			LitVarExpression* expr = (LitVarExpression*) expression;

			reallocate(compiler, (void*) expr->name, strlen(expr->name) + 1, 0);
			reallocate(compiler, (void*) expression, sizeof(LitVarExpression), 0);

			break;
		}
		case ASSIGN_EXPRESSION: {
			LitAssignExpression* expr = (LitAssignExpression*) expression;

			lit_free_expression(compiler, expr->value);
			lit_free_expression(compiler, expr->to);
			reallocate(compiler, (void*) expression, sizeof(LitAssignExpression), 0);

			break;
		}
		case LOGICAL_EXPRESSION: {
			LitLogicalExpression* expr = (LitLogicalExpression*) expression;

			lit_free_expression(compiler, expr->left);
			lit_free_expression(compiler, expr->right);
			reallocate(compiler, (void*) expression, sizeof(LitLogicalExpression), 0);

			break;
		}
		case CALL_EXPRESSION: {
			LitCallExpression* expr = (LitCallExpression*) expression;
			lit_free_expression(compiler, expr->callee);

			for (int i = 0; i < expr->args->count; i++) {
				lit_free_expression(compiler, expr->args->values[i]);
			}

			lit_free_expressions(compiler, expr->args);
			reallocate(compiler, (void*) expr->args, sizeof(LitExpressions), 0);
			reallocate(compiler, (void*) expression, sizeof(LitCallExpression), 0);

			break;
		}
		case LAMBDA_EXPRESSION: {
			LitLambdaExpression* expr = (LitLambdaExpression*) expression;

			if (strcmp(expr->return_type.type, "void") != 0) {
				reallocate(compiler, (void*) expr->return_type.type, strlen(expr->return_type.type) + 1, 0);
			}

			lit_free_statement(compiler, expr->body);

			if (expr->parameters != NULL) {
				for (int i = 0; i  < expr->parameters->count; i++) {
					LitParameter parameter = expr->parameters->values[i];

					reallocate(compiler, (void*) parameter.name, strlen(parameter.name) + 1, 0);
					reallocate(compiler, (void*) parameter.type, strlen(parameter.type) + 1, 0);
				}

				lit_free_parameters(compiler, expr->parameters);
				reallocate(compiler, (void*) expr->parameters, sizeof(LitParameters), 0);
			}

			reallocate(compiler, (void*) expression, sizeof(LitLambdaExpression), 0);
			break;
		}
		case GET_EXPRESSION: {
			LitGetExpression* expr = (LitGetExpression*) expression;

			lit_free_expression(compiler, expr->object);
			reallocate(compiler, (void*) expr->property, strlen(expr->property) + 1, 0);
			reallocate(compiler, (void*) expression, sizeof(LitGetExpression), 0);

			break;
		}
		case SET_EXPRESSION: {
			LitSetExpression* expr = (LitSetExpression*) expression;

			lit_free_expression(compiler, expr->object);
			lit_free_expression(compiler, expr->value);
			reallocate(compiler, (void*) expr->property, strlen(expr->property) + 1, 0);
			reallocate(compiler, (void*) expression, sizeof(LitSetExpression), 0);

			break;
		}
		case THIS_EXPRESSION: {
			reallocate(compiler, (void*) expression, sizeof(LitThisExpression), 0);

			break;
		}
		case SUPER_EXPRESSION: {
			LitSuperExpression* expr = (LitSuperExpression*) expression;

			reallocate(compiler, (void*) expr->method, strlen(expr->method) + 1, 0);
			reallocate(compiler, (void*) expression, sizeof(LitSuperExpression), 0);

			break;
		}
		case IF_EXPRESSION: {
			LitIfExpression* expr = (LitIfExpression*) expression;

			lit_free_expression(compiler, expr->condition);
			lit_free_expression(compiler, expr->if_branch);

			if (expr->else_if_branches != NULL) {
				for (int i = 0; i < expr->else_if_branches->count; i++) {
					lit_free_expression(compiler, expr->else_if_conditions->values[i]);
					lit_free_expression(compiler, expr->else_if_branches->values[i]);
				}

				lit_free_expressions(compiler, expr->else_if_conditions);
				lit_free_expressions(compiler, expr->else_if_branches);

				reallocate(compiler, (void*) expr->else_if_conditions, sizeof(LitExpressions), 0);
				reallocate(compiler, (void*) expr->else_if_branches, sizeof(LitExpressions), 0);
			}

			if (expr->else_branch != NULL) {
				lit_free_expression(compiler, expr->else_branch);
			}

			reallocate(compiler, (void*) expr, sizeof(LitIfExpression), 0);
			break;
		}
		default: {
			fflush(stdin);
			fprintf(stderr, "Expression with id %i has no freeing case!\n", expression->type);
			fflush(stdout);

			break;
		}
	}
}