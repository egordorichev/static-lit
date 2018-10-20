#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <zconf.h>
#include <lit.h>

#include "lit_resolver.h"
#include "lit_table.h"
#include "lit_memory.h"
#include "lit_object.h"

DEFINE_ARRAY(LitScopes, LitLetals*, scopes)
DEFINE_ARRAY(LitStrings, char*, strings)

DEFINE_TABLE(LitLetals, LitLetal*, letals, NULL);
DEFINE_TABLE(LitTypes, char *, types, NULL)
DEFINE_TABLE(LitClasses, LitClass*, classes, NULL)

static void resolve_statement(LitResolver* resolver, LitStatement* statement);
static void resolve_statements(LitResolver* resolver, LitStatements* statements);

static const char* resolve_expression(LitResolver* resolver, LitExpression* expression);
static void resolve_expressions(LitResolver* resolver, LitExpressions* expressions);

static void error(LitResolver* resolver, const char* format, ...) {
	va_list vargs;
	va_start(vargs, format);
	fprintf(stderr, "Error: ");
	vfprintf(stderr, format, vargs);
	fprintf(stderr, "\n");
	va_end(vargs);

	resolver->had_error = true;
}

static bool compare_arg(char* needed, char* given) {
	if (strcmp(given, needed) == 0 || strcmp(needed, "any") == 0) {
		return true;
	}

	if ((strcmp(given, "double") == 0 || strcmp(given, "int") == 0) && (strcmp(needed, "double") == 0 || strcmp(needed, "int") == 0)) {
		return true;
	}

	return false;
}

static void push_scope(LitResolver* resolver) {
	LitLetals* table = (LitLetals*) reallocate(resolver->vm, NULL, 0, sizeof(LitLetals));
	lit_init_letals(table);
	lit_scopes_write(resolver->vm, &resolver->scopes, table);

	resolver->depth ++;
}

static LitLetals* peek_scope(LitResolver* resolver) {
	return resolver->scopes.values[resolver->depth - 1];
}

static void pop_scope(LitResolver* resolver) {
	resolver->scopes.count --;
	LitLetals* table = resolver->scopes.values[resolver->scopes.count];

	for (int i = 0; i <= table->capacity_mask; i++) {
		LitLetal* value = table->entries[i].value;

		if (value != NULL) {
			reallocate(resolver->vm, (void*) value, sizeof(LitLetal), 0);
		}
	}

	lit_free_letals(resolver->vm, table);
	reallocate(resolver->vm, table, sizeof(LitLetals), 0);

	resolver->depth --;
}

static size_t strlen_ignoring(const char *str) {
	const char *s;
	for (s = str; *s && *s != '<'; ++s);

	return s - str;
}

static void resolve_type(LitResolver* resolver, const char* type) {
	if (lit_types_get(&resolver->types, lit_copy_string(resolver->vm, type, (int) strlen_ignoring(type))) == NULL) {
		error(resolver, "Type %s is not defined", type);
	}
}

static void define_type(LitResolver* resolver, const char* type) {
	LitString* str = lit_copy_string(resolver->vm, type, (int) strlen(type));
	lit_types_set(resolver->vm, &resolver->types, str, (char*) type);
}

// FIXME: no need to allocate this str, cause
// in define_type we use lit_copy_string that allocates a new string
// the resolver->types should be a table of string and bool
static void define_type_allocating(LitResolver* resolver, const char* type) {
	size_t len = strlen(type) + 1;
	char* str = reallocate(resolver->vm, NULL, 0, len);
	strncpy(str, type, len);
	str[len - 1] = '\0';

	define_type(resolver, str);
}

void lit_init_letal(LitLetal* letal) {
	letal->type = NULL;
	letal->defined = false;
	letal->nil = false;
	letal->field = false;
}

static LitLetal* resolve_local(LitResolver* resolver, const char* name) {
	LitString *str = lit_copy_string(resolver->vm, name, (int) strlen(name));

	for (int i = resolver->scopes.count - 1; i >= 0; i --) {
		LitLetal** value = lit_letals_get(resolver->scopes.values[i], str);

		if (!(value == NULL || (*value)->nil)) {
			return *value;
		}
	}

	LitLetal** value = lit_letals_get(&resolver->externals, str);

	if (!(value == NULL || (*value)->nil)) {
		return *value;
	}

	error(resolver, "Variable %s is not defined", name);
	return NULL;
}

static void declare(LitResolver* resolver, const char* name) {
	LitLetals* scope = peek_scope(resolver);
	LitString* str = lit_copy_string(resolver->vm, name, (int) strlen(name));
	LitLetal** value = lit_letals_get(scope, str);

	if (value != NULL) {
		error(resolver, "Variable %s is already defined in current scope", name);
	}

	LitLetal* letal = (LitLetal*) reallocate(resolver->vm, NULL, 0, sizeof(LitLetal));

	lit_init_letal(letal);
	lit_letals_set(resolver->vm, scope, str, letal);
}

static void declare_and_define(LitResolver* resolver, const char* name, const char* type) {
	LitLetals* scope = peek_scope(resolver);
	LitString* str = lit_copy_string(resolver->vm, name, (int) strlen(name));
	LitLetal** value = lit_letals_get(scope, str);

	if (value != NULL) {
		error(resolver, "Variable %s is already defined in current scope", name);
	} else {
		LitLetal* letal = (LitLetal*) reallocate(resolver->vm, NULL, 0, sizeof(LitLetal));
		lit_init_letal(letal);

		letal->defined = true;
		letal->type = type;

		lit_letals_set(resolver->vm, scope, str, letal);
	}
}

static void define(LitResolver* resolver, const char* name, const char* type, bool field) {
	LitLetals* scope = peek_scope(resolver);
	LitString* str = lit_copy_string(resolver->vm, name, (int) strlen(name));

	LitLetal** value = lit_letals_get(scope, str);

	if (value == NULL) {
		LitLetal* letal = (LitLetal*) reallocate(resolver->vm, NULL, 0, sizeof(LitLetal));

		lit_init_letal(letal);

		letal->defined = true;
		letal->type = type;
		letal->field = field;

		lit_letals_set(resolver->vm, scope, str, value);
	} else {
		LitLetal* letal = *value;

		letal->defined = true;
		letal->type = type;
		letal->field = field;
	}
}

static const char* resolve_var_statement(LitResolver* resolver, LitVarStatement* statement) {
	declare(resolver, statement->name);
	char *type = "void";

	if (statement->init != NULL) {
		type = (char*) resolve_expression(resolver, statement->init);
	}

	if (strcmp(type, "void") == 0) {
		error(resolver, "Can't set variable's %s type to void", statement->name);
	} else {
		define(resolver, statement->name, type, resolver->class != NULL && resolver->depth == 2);
	}

	return type;
}

static void resolve_expression_statement(LitResolver* resolver, LitExpressionStatement* statement) {
	resolve_expression(resolver, statement->expr);
}

static void resolve_if_statement(LitResolver* resolver, LitIfStatement* statement) {
	resolve_expression(resolver, statement->condition);
	resolve_statement(resolver, statement->if_branch);

	if (statement->else_if_branches != NULL) {
		resolve_expressions(resolver, statement->else_if_conditions);
		resolve_statements(resolver, statement->else_if_branches);
	}

	if (statement->else_branch != NULL) {
		resolve_statement(resolver, statement->else_branch);
	}
}

static void resolve_block_statement(LitResolver* resolver, LitBlockStatement* statement) {
	if (statement->statements != NULL) {
		push_scope(resolver);
		resolve_statements(resolver, statement->statements);
		pop_scope(resolver);
	}
}

static void resolve_while_statement(LitResolver* resolver, LitWhileStatement* statement) {
	resolve_expression(resolver, statement->condition);
	resolve_statement(resolver, statement->body);
}

static void resolve_function(LitResolver* resolver, LitParameters* parameters, LitParameter* return_type, LitStatement* body,
	const char* message, const char* name) {

	push_scope(resolver);

	resolver->had_return = false;

	if (parameters != NULL) {
		for (int i = 0; i < parameters->count; i++) {
			LitParameter parameter = parameters->values[i];
			resolve_type(resolver, parameter.type);
			define(resolver, parameter.name, parameter.type, false);
		}
	}

	resolve_type(resolver, return_type->type);
	resolve_statement(resolver, body);

	if (!resolver->had_return) {
		if (strcmp(return_type->type, "void") != 0) {
			error(resolver, message, name);
		} else {
			LitBlockStatement* block = (LitBlockStatement*) body;

			if (block->statements == NULL) {
				block->statements = (LitStatements*) reallocate(resolver->vm, NULL, 0, sizeof(LitStatements));
				lit_init_statements(block->statements);
			}

			lit_statements_write(resolver->vm, block->statements, (LitStatement*) lit_make_return_statement(resolver->vm, NULL));
		}
	}

	pop_scope(resolver);
}

static const char* get_function_signature(LitResolver* resolver, LitParameters* parameters, LitParameter* return_type) {
	size_t len = 11;

	if (parameters != NULL) {
		int cn = parameters->count;

		for (int i = 0; i < cn; i++) {
			len += strlen(parameters->values[i].type) + 2; // ', '
		}
	}

	len += strlen(return_type->type);
	char* type = (char*) reallocate(resolver->vm, NULL, 0, len);

	strncpy(type, "function<", 9);
	int place = 9;

	if (parameters != NULL) {
		int cn = parameters->count;

		for (int i = 0; i < cn; i++) {
			const char* tp = parameters->values[i].type;
			size_t l = strlen(tp);

			strncpy(&type[place], tp, l);
			place += l;
			strncpy(&type[place], ", ", 2);
			place += 2; // ', '
		}
	}

	size_t l = strlen(return_type->type); // minus \0
	strncpy(&type[place], return_type->type, l);

	type[len - 2] = '>';
	type[len - 1] = '\0';

	return type;
}

static void resolve_function_statement(LitResolver* resolver, LitFunctionStatement* statement) {
	const char* type = get_function_signature(resolver, statement->parameters, &statement->return_type);

	LitFunctionStatement* last = resolver->function;
	resolver->function = statement;

	declare_and_define(resolver, statement->name, type);
	resolve_function(resolver, statement->parameters, &statement->return_type, statement->body, "Missing return statement in function %s", statement->name);

	resolver->function = last;

	reallocate(resolver->vm, (void*) type, strlen(type) + 1, 0);
}

static void resolve_return_statement(LitResolver* resolver, LitReturnStatement* statement) {
	char* type = (char*) (statement->value == NULL ? "void" : resolve_expression(resolver, statement->value));
	resolver->had_return = true;

	if (resolver->function == NULL) {
		error(resolver, "Can't return from top-level code!");
	} else if (!compare_arg((char*) resolver->function->return_type.type, type)) {
		error(resolver, "Return type mismatch: required %s, but got %s", resolver->function->return_type.type, type);
	}
}

static const char* resolve_var_expression(LitResolver* resolver, LitVarExpression* expression);

static void resolve_method_statement(LitResolver* resolver, LitFunctionStatement* statement) {
	push_scope(resolver);
	resolver->had_return = false;

	if (statement->parameters != NULL) {
		for (int i = 0; i < statement->parameters->count; i++) {
			LitParameter parameter = statement->parameters->values[i];
			resolve_type(resolver, parameter.type);
			define(resolver, parameter.name, parameter.type, false);
		}
	}

	resolve_type(resolver, statement->return_type.type);

	LitFunctionStatement* enclosing = resolver->function;

	resolver->function = statement;
	resolve_statement(resolver, statement->body);
	resolver->function = enclosing;

	if (!resolver->had_return) {
		if (strcmp(statement->return_type.type, "void") != 0) {
			error(resolver, "Missing return statement in method %s", statement->name);
		} else {
			LitBlockStatement* block = (LitBlockStatement*) statement->body;

			if (block->statements == NULL) {
				block->statements = (LitStatements*) reallocate(resolver->vm, NULL, 0, sizeof(LitStatements));
				lit_init_statements(block->statements);
			}

			lit_statements_write(resolver->vm, block->statements, (LitStatement*) lit_make_return_statement(resolver->vm, NULL));
		}
	}

	pop_scope(resolver);
}

static void resolve_class_statement(LitResolver* resolver, LitClassStatement* statement) {
	printf("%s\n", statement->name);

	size_t len = strlen(statement->name);
	char* type = (char*) reallocate(resolver->vm, NULL, 0, 7 + len);

	strncpy(type, "class<", 6);
	strncpy(type + 6, statement->name, len);
	type[6 + len] = '>';
	type[7 + len] = '\0';

	LitString* name = lit_copy_string(resolver->vm, statement->name, len);

	define_type(resolver, name->chars);
	declare_and_define(resolver, name->chars, type);

	if (statement->super != NULL) {
		resolve_var_expression(resolver, statement->super);
	}

	LitClass* super = NULL;

	if (statement->super != NULL) {
		LitClass** super_class = lit_classes_get(&resolver->classes, lit_copy_string(resolver->vm, statement->super->name, strlen(statement->super->name)));

		if (super_class == NULL) {
			error(resolver, "Can't inherit undefined class %s", statement->super->name);
		} else {
			super = *super_class;
		}
	}

	LitClass* class = lit_new_class(resolver->vm, name, super);

	if (super != NULL) {
		lit_table_add_all(resolver->vm, &class->methods, &super->methods);
		lit_fields_add_all(resolver->vm, &class->fields, &super->fields);
	}

	resolver->class = class;
	lit_classes_set(resolver->vm, &resolver->classes, name, class);
	push_scope(resolver);

	if (statement->fields != NULL) {
		for (int i = 0; i < statement->fields->count; i++) {
			const char* name = ((LitVarStatement*) statement->fields->values[i])->name;
			const char* type = resolve_var_statement(resolver, (LitVarStatement*) statement->fields->values[i]);

			lit_fields_set(resolver->vm, &class->fields, lit_copy_string(resolver->vm, name, strlen(name)), (LitField) { 0, type });
		}
	}

	if (statement->methods != NULL) {
		for (int i = 0; i < statement->methods->count; i++) {
			LitFunctionStatement* method = statement->methods->values[i];
			resolve_method_statement(resolver, method);

			LitMethod *m = lit_new_bound_method(resolver->vm, 0, NULL);
			m->signature = (char*) get_function_signature(resolver, method->parameters, &method->return_type);

			lit_table_set(resolver->vm, &class->methods, lit_copy_string(resolver->vm, method->name, strlen(method->name)), MAKE_OBJECT_VALUE(m));
		}
	}

	pop_scope(resolver);
	reallocate(resolver->vm, (void*) type, 7 + len, 0);
}

static void resolve_statement(LitResolver* resolver, LitStatement* statement) {
	switch (statement->type) {
		case VAR_STATEMENT: resolve_var_statement(resolver, (LitVarStatement*) statement); break;
		case EXPRESSION_STATEMENT: resolve_expression_statement(resolver, (LitExpressionStatement*) statement); break;
		case IF_STATEMENT: resolve_if_statement(resolver, (LitIfStatement*) statement); break;
		case BLOCK_STATEMENT: resolve_block_statement(resolver, (LitBlockStatement*) statement); break;
		case WHILE_STATEMENT: resolve_while_statement(resolver, (LitWhileStatement*) statement); break;
		case FUNCTION_STATEMENT: resolve_function_statement(resolver, (LitFunctionStatement*) statement); break;
		case RETURN_STATEMENT: resolve_return_statement(resolver, (LitReturnStatement*) statement); break;
		case CLASS_STATEMENT: resolve_class_statement(resolver, (LitClassStatement*) statement); break;
	}
}

static void resolve_statements(LitResolver* resolver, LitStatements* statements) {
	for (int i = 0; i < statements->count; i++) {
		resolve_statement(resolver, statements->values[i]);
	}
}

static const char* resolve_binary_expression(LitResolver* resolver, LitBinaryExpression* expression) {
	const char* a = resolve_expression(resolver, expression->left);
	const char* b = resolve_expression(resolver, expression->right);

	if (!((strcmp(a, "int") == 0 || strcmp(a, "double") == 0) && (strcmp(b, "int") == 0 || strcmp(b, "double") == 0))) {
		error(resolver, "Can't perform binary operation on %s and %s", a, b);
	}

	return a;
}

static const char* resolve_literal_expression(LitResolver* resolver, LitLiteralExpression* expression) {
	if (IS_NUMBER(expression->value)) {
		double number = AS_NUMBER(expression->value);
		double temp;

		if (modf(number, &temp) == 0) {
			return "int";
		}

		return "double";
	} else if (IS_BOOL(expression->value)) {
		return "bool";
	} else if (IS_CHAR(expression->value)) {
		return "char";
	} else if (IS_STRING(expression->value)) {
		return "String";
	}

	// nil
	return "error";
}

static const char* resolve_unary_expression(LitResolver* resolver, LitUnaryExpression* expression) {
	const char* type = resolve_expression(resolver, expression->right);

	if (expression->operator == TOKEN_MINUS && (strcmp(type, "int") != 0 && strcmp(type, "double") != 0)) {
		// TODO: easter egg about muffin?
		error(resolver, "Can't negate non-number values");
		return "error";
	}

	return type;
}

static const char* resolve_grouping_expression(LitResolver* resolver, LitGroupingExpression* expression) {
	return resolve_expression(resolver, expression->expr);
}

static const char* resolve_var_expression(LitResolver* resolver, LitVarExpression* expression) {
	LitLetal** value = lit_letals_get(peek_scope(resolver), lit_copy_string(resolver->vm, expression->name, (int) strlen(expression->name)));

	if (value != NULL && !(*value)->defined) {
		error(resolver, "Can't use local variable %s in it's own initializer", expression->name);
		return "error";
	}

	LitLetal* letal = resolve_local(resolver, expression->name);
	
	if (letal == NULL) {
		return "error";
	} else if (letal->field && resolver->class != NULL && resolver->depth > 2) {
		error(resolver, "Can't access class field %s without this", expression->name);
		return "error";
	}

	return letal->type;
}

static const char* resolve_assign_expression(LitResolver* resolver, LitAssignExpression* expression) {
	const char* given = resolve_expression(resolver, expression->value);
	const char* type = resolve_expression(resolver, expression->to);

	if (!compare_arg((char*) type, (char*) given)) {
		error(resolver, "Can't assign %s value to a %s var", given, type);
	}

	LitVarExpression* expr = (LitVarExpression*) expression->to;
	LitLetal* letal = resolve_local(resolver, expr->name);

	if (letal == NULL) {
		return "error";
	}

	return letal->type;
}

static const char* resolve_logical_expression(LitResolver* resolver, LitLogicalExpression* expression) {
	return resolve_expression(resolver, expression->right);
}

static char* last_string;
static bool had_template;

static char* tok(char* string) {
	char* start = string == NULL ? last_string : string;

	if (!*start || *start == '>') {
		return NULL;
	}

	if (*start == ' ') {
		start++;
	}

	char* where_started = start;

	while (*start != '\0' && *start != ',' && *start != '>') {
		start++;

		if (*start == '<') {
			while (*start != '>') {
				start++;
			}

			start++;
		}
	}

	had_template = (*start == '>');

	*start = '\0';
	last_string = start + 1;

	return where_started;
}

int strcmp_ignoring(const char* s1, const char* s2) {
	while(*s1 && *s1 != '<' && *s2 != '<' && *s1 == *s2) {
		s1++;
		s2++;
	}

	return *s1 > *s2 ? 1 : (*s1 == *s2 ? 0 : -1);
}

static const char* extract_callee_name(LitExpression* expression) {
	switch (expression->type) {
		case VAR_EXPRESSION: return ((LitVarExpression*) expression)->name;
		case GET_EXPRESSION: return ((LitGetExpression*) expression)->property;
		case SET_EXPRESSION: return ((LitSetExpression*) expression)->property;
		case GROUPING_EXPRESSION: return extract_callee_name(((LitGroupingExpression*) expression)->expr);
		case SUPER_EXPRESSION: return ((LitSuperExpression*) expression)->method;
	}

	return NULL;
}

static const char* resolve_call_expression(LitResolver* resolver, LitCallExpression* expression) {
	char* return_type = "void";

	if (expression->callee->type != VAR_EXPRESSION && expression->callee->type != GET_EXPRESSION && expression->callee->type != GROUPING_EXPRESSION
		&& expression->callee->type != SET_EXPRESSION && expression->callee->type != SUPER_EXPRESSION) {

		error(resolver, "Can't call non-variable of type %i", expression->callee->type);
	} else {
		const char* type = resolve_expression(resolver, expression->callee);
		const char* name = extract_callee_name(expression->callee);

		if (strcmp_ignoring(type, "class<") == 0) {
			size_t len = strlen(type);
			return_type = (char*) reallocate(resolver->vm, NULL, 0, len - 7);
			strncpy(return_type, &type[6], len - 7);
		} else if (strcmp_ignoring(type, "function<") != 0) {
			error(resolver, "Can't call non-function variable %s", name);
		} else {
			if (strcmp(type, "error") == 0) {
				error(resolver, "Can't call non-defined function %s", name);
				return "error";
			}

			size_t len = strlen(type);
			char* tp = (char*) reallocate(resolver->vm, NULL, 0, len);
			strncpy(tp, type, len);

			char* arg = tok(&tp[9]);
			int i = 0;
			int cn = expression->args->count;

			while (arg != NULL) {
				if (!had_template && i >= cn) {
					error(resolver, "Not enough arguments for %s, expected %i, got %i, for function %s", type, i + 1, cn, name);
					break;
				}

				if (!had_template) {
					const char* given_type = resolve_expression(resolver, expression->args->values[i]);

					if (given_type == NULL) {
						error(resolver, "Got null type resolved somehow");
					} else if (!compare_arg(arg, (char*) given_type)) {
						error(resolver, "Argument #%i type mismatch: required %s, but got %s, for function %s", i + 1, arg, given_type, name);
					}
				} else {
					size_t len = strlen(arg);
					return_type = (char*) reallocate(resolver->vm, NULL, 0, len + 1);
					strncpy(return_type, arg, len);
					return_type[len] = '\0';

					lit_strings_write(resolver->vm, &resolver->allocated_strings, return_type);

					break;
				}

				arg = tok(NULL);
				i++;

				if (arg == NULL) {
					break;
				}
			}

			reallocate(resolver->vm, tp, len, 0);

			if (i < cn) {
				error(resolver, "Too many arguments for function %s, expected %i, got %i, for function %s", type, i, cn, ((LitVarExpression*) expression->callee)->name);
			}
		}
	}

	return return_type;
}

static const char* resolve_get_expression(LitResolver* resolver, LitGetExpression* expression) {
	const char* type = resolve_expression(resolver, expression->object);
	LitClass** class = lit_classes_get(&resolver->classes, lit_copy_string(resolver->vm, type, strlen(type)));

	if (class == NULL) {
		error(resolver, "Can't find class %s", type);
		return "error";
	}

	LitString* str = lit_copy_string(resolver->vm, expression->property, strlen(expression->property));
	LitField* field = lit_fields_get(&(*class)->fields, str);

	if (field == NULL) {
		LitValue* value = lit_table_get(&(*class)->methods, str);

		if (value == NULL) {
			error(resolver, "Class %s has no field or method %s", type, expression->property);
			return "error";
		}

		return AS_METHOD(*value)->signature;
	}

	return field->type;
}

static const char* resolve_set_expression(LitResolver* resolver, LitSetExpression* expression) {
	const char* type = resolve_expression(resolver, expression->object);
	LitClass* class = *lit_classes_get(&resolver->classes, lit_copy_string(resolver->vm, type, strlen(type)));

	if (class == NULL) {
		error(resolver, "Undefined type %s", type);
		return "error";
	}

	LitField* field = lit_fields_get(&class->fields, lit_copy_string(resolver->vm, expression->property, strlen(expression->property)));

	if (field == NULL) {
		error(resolver, "Class %s has no field %s", type, expression->property);
		return "error";
	}

	const char* var_type = expression->value == NULL ? "void" : resolve_expression(resolver, expression->value);

	if (!compare_arg((char*) field->type, (char*) var_type)) {
		error(resolver, "Can't assign %s value to %s field %s", var_type, field->type, expression->property);
		return "error";
	}

	return field->type;
}

static const char* resolve_lambda_expression(LitResolver* resolver, LitLambdaExpression* expression) {
	const char* type = get_function_signature(resolver, expression->parameters, &expression->return_type);
	LitFunctionStatement* last = resolver->function;

	resolver->function = (LitFunctionStatement*) expression; // HACKZ
	resolve_function(resolver, expression->parameters, &expression->return_type, expression->body, "Missing return statement in lambda", NULL);
	resolver->function = last;

	reallocate(resolver->vm, (void*) type, strlen(type) + 1, 0);
	return type;
}

static const char* resolve_this_expression(LitResolver* resolver, LitThisExpression* expression) {
	if (resolver->class == NULL) {
		error(resolver, "Can't use this outside of a class");
		return "error";
	}

	return resolver->class->name->chars;
}

static const char* resolve_super_expression(LitResolver* resolver, LitSuperExpression* expression) {
	if (resolver->class == NULL) {
		error(resolver, "Can't use super outside of a class");
		return "error";
	}

	if (resolver->class->super == NULL) {
		error(resolver, "Class %s has no super", resolver->class->name->chars);
		return "error";
	}

	LitValue* method = lit_table_get(&resolver->class->super->methods, lit_copy_string(resolver->vm, expression->method, strlen(expression->method)));

	if (method == NULL) {
		error(resolver, "Class %s has no method %s", resolver->class->super->name->chars, expression->method);
		return "error";
	}

	return AS_METHOD(*method)->signature;
}

static const char* resolve_expression(LitResolver* resolver, LitExpression* expression) {
	switch (expression->type) {
		case BINARY_EXPRESSION: return resolve_binary_expression(resolver, (LitBinaryExpression*) expression);
		case LITERAL_EXPRESSION: return resolve_literal_expression(resolver, (LitLiteralExpression*) expression);
		case UNARY_EXPRESSION: return resolve_unary_expression(resolver, (LitUnaryExpression*) expression);
		case GROUPING_EXPRESSION: return resolve_grouping_expression(resolver, (LitGroupingExpression*) expression);
		case VAR_EXPRESSION: return resolve_var_expression(resolver, (LitVarExpression*) expression);
		case ASSIGN_EXPRESSION: return resolve_assign_expression(resolver, (LitAssignExpression*) expression);
		case LOGICAL_EXPRESSION: return resolve_logical_expression(resolver, (LitLogicalExpression*) expression);
		case CALL_EXPRESSION: return resolve_call_expression(resolver, (LitCallExpression*) expression);
		case GET_EXPRESSION: return resolve_get_expression(resolver, (LitGetExpression*) expression);
		case SET_EXPRESSION: return resolve_set_expression(resolver, (LitSetExpression*) expression);
		case LAMBDA_EXPRESSION: return resolve_lambda_expression(resolver, (LitLambdaExpression*) expression);
		case THIS_EXPRESSION: return resolve_this_expression(resolver, (LitThisExpression*) expression);
		case SUPER_EXPRESSION: return resolve_super_expression(resolver, (LitSuperExpression*) expression);
	}

	return "error";
}

static void resolve_expressions(LitResolver* resolver, LitExpressions* expressions) {
	for (int i = 0; i < expressions->count; i++) {
		resolve_expression(resolver, expressions->values[i]);
	}
}

void lit_init_resolver(LitResolver* resolver) {
	lit_init_scopes(&resolver->scopes);
	lit_init_types(&resolver->types);
	lit_init_classes(&resolver->classes);
	lit_init_strings(&resolver->allocated_strings);

	resolver->had_error = false;
	resolver->depth = 0;
	resolver->function = NULL;
	resolver->class = NULL;

	push_scope(resolver); // Global scope

	define_type_allocating(resolver, "int");
	define_type_allocating(resolver, "bool");
	define_type_allocating(resolver, "error");
	define_type_allocating(resolver, "void");
	define_type_allocating(resolver, "any");
	define_type_allocating(resolver, "double");
	define_type_allocating(resolver, "char");
	define_type_allocating(resolver, "function");
	define_type_allocating(resolver, "Class");
	define_type_allocating(resolver, "String");
}

void lit_free_resolver(LitResolver* resolver) {
	pop_scope(resolver);

	for (int i = 0; i < resolver->allocated_strings.count; i++) {
		char* str = resolver->allocated_strings.values[i];
		reallocate(resolver->vm, (void*) str, strlen(str) + 1, 0);
	}

	lit_free_strings(resolver->vm, &resolver->allocated_strings);

	for (int i = 0; i <= resolver->types.capacity_mask; i++) {
		char* type = resolver->types.entries[i].value;

		if (type != NULL) {
			reallocate(resolver->vm, type, strlen(type) + 1, 0);
		}
	}

	lit_free_types(resolver->vm, &resolver->types);
	lit_free_scopes(resolver->vm, &resolver->scopes);
	lit_free_classes(resolver->vm, &resolver->classes);
}

bool lit_resolve(LitVm* vm, LitStatements* statements) {
	lit_init_resolver(&vm->resolver);
	resolve_statements(&vm->resolver, statements);
	lit_free_resolver(&vm->resolver);

	return !vm->resolver.had_error;
}