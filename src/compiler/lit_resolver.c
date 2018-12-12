#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <zconf.h>
#include <lit.h>

#include <compiler/lit_resolver.h>
#include <compiler/lit_compiler.h>

#include <util/lit_table.h>
#include <vm/lit_memory.h>
#include <vm/lit_object.h>
#include <compiler/lit_ast.h>

DEFINE_ARRAY(LitScopes, LitResolverLocals*, scopes)
DEFINE_ARRAY(LitStrings, char*, strings)

DEFINE_TABLE(LitResolverLocals, LitResolverLocal*, resolver_locals, LitResolverLocal*, NULL, entry->value);
DEFINE_TABLE(LitTypes, bool, types, bool, false, entry->value)
DEFINE_TABLE(LitClasses, LitType*, classes, LitType*, NULL, entry->value)
DEFINE_TABLE(LitResolverFields, LitResolverField*, resolver_fields, LitResolverField*, NULL, entry->value)
DEFINE_TABLE(LitResolverMethods, LitResolverMethod*, resolver_methods, LitResolverMethod*, NULL, entry->value)

static void resolve_statement(LitResolver* resolver, LitStatement* statement);
static void resolve_statements(LitResolver* resolver, LitStatements* statements);

static const char* resolve_expression(LitResolver* resolver, LitExpression* expression);
static void resolve_expressions(LitResolver* resolver, LitExpressions* expressions);

static void error(LitResolver* resolver, uint64_t line, const char* format, ...) {
	fflush(stdout);

	va_list vargs;
	va_start(vargs, format);
	fprintf(stderr, "[line %lu] Error: ", line);
	vfprintf(stderr, format, vargs);
	fprintf(stderr, "\n");
	va_end(vargs);

	fflush(stderr);
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
	LitResolverLocals* table = (LitResolverLocals*) reallocate(resolver->compiler, NULL, 0, sizeof(LitResolverLocals));
	lit_init_resolver_locals(table);
	lit_scopes_write(resolver->compiler, &resolver->scopes, table);

	resolver->depth ++;
}

static LitResolverLocals* peek_scope(LitResolver* resolver) {
	return resolver->scopes.values[resolver->depth - 1];
}

static void pop_scope(LitResolver* resolver) {
	resolver->scopes.count --;
	LitResolverLocals* table = resolver->scopes.values[resolver->scopes.count];

	for (int i = 0; i <= table->capacity_mask; i++) {
		LitResolverLocal* value = table->entries[i].value;

		if (value != NULL) {
			reallocate(resolver->compiler, (void*) value, sizeof(LitResolverLocal), 0);
		}
	}

	lit_free_resolver_locals(resolver->compiler, table);
	reallocate(resolver->compiler, table, sizeof(LitResolverLocals), 0);

	resolver->depth --;
}

static size_t strlen_ignoring(const char *str) {
	const char *s;
	for (s = str; *s && *s != '<'; ++s);

	return s - str;
}

static void resolve_type(LitResolver* resolver, const char* type, uint64_t line) {
	if (!lit_types_get(&resolver->types, lit_copy_string(resolver->compiler, type, (int) strlen_ignoring(type)))) {
		error(resolver, line, "Type %s is not defined", type);
	}
}

void lit_define_type(LitResolver* resolver, const char* type) {
	LitString* str = lit_copy_string(resolver->compiler, type, (int) strlen(type));
	lit_types_set(resolver->compiler, &resolver->types, str, true);
}

void lit_init_resolver_local(LitResolverLocal* local) {
	local->type = NULL;
	local->defined = false;
	local->nil = false;
	local->field = false;
	local->final = false;
}

static LitResolverLocal* resolve_local(LitResolver* resolver, const char* name, uint64_t line) {
	LitString *str = lit_copy_string(resolver->compiler, name, (int) strlen(name));

	for (int i = resolver->scopes.count - 1; i >= 0; i --) {
		LitResolverLocal* value = lit_resolver_locals_get(resolver->scopes.values[i], str);

		if (value != NULL && !value->nil) {
			return value;
		}
	}

	LitResolverLocal* value = lit_resolver_locals_get(&resolver->externals, str);

	if (value != NULL && !value->nil) {
		return value;
	}

	error(resolver, line, "Variable %s is not defined", name);
	return NULL;
}

static void declare(LitResolver* resolver, const char* name) {
	LitResolverLocals* scope = peek_scope(resolver);
	LitString* str = lit_copy_string(resolver->compiler, name, (int) strlen(name));
	LitResolverLocal* value = lit_resolver_locals_get(scope, str);

	if (value != NULL) {
		error(resolver, "Variable %s is already defined in current scope", name);
	}

	LitResolverLocal* local = (LitResolverLocal*) reallocate(resolver->compiler, NULL, 0, sizeof(LitResolverLocal));

	lit_init_resolver_local(local);
	lit_resolver_locals_set(resolver->compiler, scope, str, local);
}

static void declare_and_define(LitResolver* resolver, const char* name, const char* type) {
	LitResolverLocals* scope = peek_scope(resolver);
	LitString* str = lit_copy_string(resolver->compiler, name, (int) strlen(name));
	LitResolverLocal* value = lit_resolver_locals_get(scope, str);

	if (value != NULL) {
		error(resolver, "Variable %s is already defined in current scope", name);
	} else {
		LitResolverLocal* local = (LitResolverLocal*) reallocate(resolver->compiler, NULL, 0, sizeof(LitResolverLocal));
		lit_init_resolver_local(local);

		local->defined = true;
		local->type = type;

		lit_resolver_locals_set(resolver->compiler, scope, str, local);
	}
}

static LitResolverLocal* define(LitResolver* resolver, const char* name, const char* type, bool field) {
	LitResolverLocals* scope = peek_scope(resolver);
	LitString* str = lit_copy_string(resolver->compiler, name, (int) strlen(name));

	LitResolverLocal* value = lit_resolver_locals_get(scope, str);

	if (value == NULL) {
		LitResolverLocal* local = (LitResolverLocal*) reallocate(resolver->compiler, NULL, 0, sizeof(LitResolverLocal));

		lit_init_resolver_local(local);

		local->defined = true;
		local->type = type;
		local->field = field;

		lit_resolver_locals_set(resolver->compiler, scope, str, local);
		return local;
	} else {
		value->defined = true;
		value->type = type;
		value->field = field;

		return value;
	}
}

static const char* resolve_var_statement(LitResolver* resolver, LitVarStatement* statement) {
	declare(resolver, statement->name);
	const char *type = statement->type == NULL ? "void" : statement->type;

	if (statement->init != NULL) {
		type = (char*) resolve_expression(resolver, statement->init);
	} else if (statement->final) {
		error(resolver, statement->statement.line, "Final variable must be assigned a value in the declaration!");
	}

	if (type == NULL || strcmp(type, "void") == 0) {
		error(resolver, statement->statement.line, "Can't set variable's %s type to void", statement->name);
	} else {
		resolve_type(resolver, type, statement->statement.line);
		define(resolver, statement->name, type, resolver->class != NULL && resolver->depth == 2)
			->final = statement->final;
	}

	if (strcmp(type, "bool") == 0) {
		statement->default_value = OP_FALSE;
	} else if (strcmp(type, "int") == 0 || strcmp(type, "double") == 0) {
		statement->default_value = OP_CONSTANT;
	} else {
		statement->default_value = OP_NIL;
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
	LitStatement* enclosing = resolver->loop;
	resolver->loop = statement;

	resolve_expression(resolver, statement->condition);
	resolve_statement(resolver, statement->body);

	resolver->loop = enclosing;
}

static void resolve_function(LitResolver* resolver, LitParameters* parameters, LitParameter* return_type, LitStatement* body,
	const char* message, const char* name, uint64_t line) {

	push_scope(resolver);

	resolver->had_return = false;

	if (parameters != NULL) {
		for (int i = 0; i < parameters->count; i++) {
			LitParameter parameter = parameters->values[i];
			resolve_type(resolver, parameter.type, line);
			define(resolver, parameter.name, parameter.type, false);
		}
	}

	resolve_type(resolver, return_type->type, line);
	resolve_statement(resolver, body);

	if (!resolver->had_return) {
		if (strcmp(return_type->type, "void") != 0) {
			error(resolver, message, name);
		} else {
			LitBlockStatement* block = (LitBlockStatement*) body;

			if (block->statements == NULL) {
				block->statements = (LitStatements*) reallocate(resolver->compiler, NULL, 0, sizeof(LitStatements));
				lit_init_statements(block->statements);
			}

			lit_statements_write(resolver->compiler, block->statements, (LitStatement*) lit_make_return_statement(resolver->compiler, line, NULL));
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
	char* type = (char*) reallocate(resolver->compiler, NULL, 0, len);

	strncpy(type, "Function<", 9);
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
	resolve_function(resolver, statement->parameters, &statement->return_type, statement->body, "Missing return statement in function %s", statement->name, statement->statement.line);

	resolver->function = last;

	lit_strings_write(resolver->compiler, &resolver->allocated_strings, (char*) type);

	if (statement->parameters != NULL && statement->parameters->count > 255) {
		error(resolver, "Function %s has more than 255 parameters", statement->name);
	}
}

static void resolve_return_statement(LitResolver* resolver, LitReturnStatement* statement) {
	char* type = (char*) (statement->value == NULL ? "void" : resolve_expression(resolver, statement->value));
	resolver->had_return = true;

	if (resolver->function == NULL) {
		error(resolver, statement->statement.line, "Can't return from top-level code!");
	} else if (!compare_arg((char*) resolver->function->return_type.type, type)) {
		error(resolver, statement->statement.line, "Return type mismatch: required %s, but got %s", resolver->function->return_type.type, type);
	}
}

static const char* resolve_var_expression(LitResolver* resolver, LitVarExpression* expression);

static const char* access_to_string(LitAccessType type) {
	switch (type) {
		case PUBLIC_ACCESS: return "public";
		case PRIVATE_ACCESS: return "private";
		case PROTECTED_ACCESS: return "protected";
		case UNDEFINED_ACCESS: return "undefined";
		default: UNREACHABLE();
	}
}

static void resolve_method_statement(LitResolver* resolver, LitMethodStatement* statement, char* signature) {
	if (statement->is_static && statement->parameters != NULL && statement->parameters->count != 0 && strcmp("init", statement->name) == 0) {
		error(resolver, statement->statement.line, "Static constructors can not have parameters");
	}

	push_scope(resolver);
	resolver->had_return = false;

	if (statement->overriden) {
		if (resolver->class->super == NULL) {
			error(resolver, statement->statement.line, "Can't override a method in a class without a base");
		} else {
			LitResolverMethod* super_method = lit_resolver_methods_get(&resolver->class->super->methods, lit_copy_string(resolver->compiler, statement->name, strlen(statement->name)));

			if (super_method == NULL) {
				error(resolver, statement->statement.line, "Can't override method %s, it does not exist in the base class", statement->name);
			} else if (super_method->is_static) {
				error(resolver, statement->statement.line, "Method %s is declared static and can not be overridden", statement->name);
			} else if (super_method->access != statement->access) {
				error(resolver, statement->statement.line, "Method %s type was declared as %s in super, but been changed to %s in child", statement->name, access_to_string(super_method->access), access_to_string(statement->access));
			} else if (strcmp(super_method->signature, signature) != 0) {
				error(resolver, statement->statement.line, "Method %s signature was declared as %s in super, but been changed to %s in child", statement->name, super_method->signature, signature);
			}
		}
	}

	if (statement->parameters != NULL) {
		for (int i = 0; i < statement->parameters->count; i++) {
			LitParameter parameter = statement->parameters->values[i];
			resolve_type(resolver, parameter.type, statement->statement.line);
			define(resolver, parameter.name, parameter.type, false);
		}

		if (statement->parameters->count > 255) {
			error(resolver, statement->statement.line, "Method %s has more than 255 parameters", statement->name);
		}
	}

	if (strcmp(statement->name, "init") == 0 && strcmp(statement->return_type.type, "void") != 0) {
		error(resolver, statement->statement.line, "Constructor must have void return type");
	}

	resolve_type(resolver, statement->return_type.type, statement->statement.line);

	if (statement->body != NULL) {
		LitFunctionStatement *enclosing = resolver->function;

		resolver->function = (LitFunctionStatement *) statement;
		resolve_statement(resolver, statement->body);
		resolver->function = enclosing;
	}

	if (!resolver->had_return) {
		if (strcmp(statement->return_type.type, "void") != 0) {
			error(resolver, statement->statement.line, "Missing return statement in method %s", statement->name);
		} else if (statement->body != NULL) {
			LitBlockStatement* block = (LitBlockStatement*) statement->body;

			if (block->statements == NULL) {
				block->statements = (LitStatements*) reallocate(resolver->compiler, NULL, 0, sizeof(LitStatements));
				lit_init_statements(block->statements);
			}

			lit_statements_write(resolver->compiler, block->statements, (LitStatement*) lit_make_return_statement(resolver->compiler, statement->statement.line, NULL));
		}
	}

	pop_scope(resolver);
}

static void resolve_field_statement(LitResolver* resolver, LitFieldStatement* statement) {
	declare(resolver, statement->name);

	if (statement->init != NULL) {
		const char* given = (char*) resolve_expression(resolver, statement->init);

		if (statement->type == NULL) {
			statement->type = given;
		} else if (strcmp(statement->type, given) != 0) {
			error(resolver, statement->statement.line, "Can't assign %s value to a %s var", given, statement->type);
		}
	} else if (statement->final) {
		error(resolver, statement->statement.line, "Final field must have a value assigned!");
	}

	resolve_type(resolver, statement->type, statement->statement.line);
	define(resolver, statement->name, statement->type, resolver->class != NULL && resolver->depth == 2);

	if (statement->getter != NULL) {
		resolve_statement(resolver, statement->getter);
	}

	if (statement->setter != NULL) {
		resolve_statement(resolver, statement->setter);
	}
}

static void resolve_class_statement(LitResolver* resolver, LitClassStatement* statement) {
	size_t len = strlen(statement->name);
	char* type = (char*) reallocate(resolver->compiler, NULL, 0, len + 8);

	strncpy(type, "Class<", 6);
	strncpy(type + 6, statement->name, len);
	type[6 + len] = '>';
	type[7 + len] = '\0';

	LitString* name = lit_copy_string(resolver->compiler, statement->name, len);

	lit_define_type(resolver, name->chars);
	declare_and_define(resolver, name->chars, type);

	if (statement->super != NULL) {
		const char* tp = resolve_var_expression(resolver, statement->super);

		if (tp != NULL && strcmp(type, tp) == 0) {
			error(resolver, statement->statement.line, "Class %s can't inherit self!", type);
		}
	}

	LitType* super = NULL;

	if (statement->super != NULL) {
		LitType* super_class = lit_classes_get(&resolver->classes, lit_copy_string(resolver->compiler, statement->super->name, strlen(statement->super->name)));

		if (super_class == NULL) {
			error(resolver, statement->statement.line, "Can't inherit undefined class %s", statement->super->name);
		} else if (super_class->final) {
			error(resolver, statement->statement.line, "Can't inherit final class %s", statement->super->name);
		} else {
			super = super_class;
		}
	}

	LitType* class = (LitType*) reallocate(resolver->compiler, NULL, 0, sizeof(LitType));

	class->inited = false;
	class->super = super;
	class->external = false;
	class->name = lit_copy_string(resolver->compiler, statement->name, len);
	class->is_static = statement->is_static;
	class->final = statement->final;
	class->abstract = statement->abstract;

	lit_init_resolver_methods(&class->methods);
	lit_init_resolver_methods(&class->static_methods);
	lit_init_resolver_fields(&class->fields);
	lit_init_resolver_fields(&class->static_fields);

	if (super != NULL) {
		lit_resolver_methods_add_all(resolver->compiler, &class->methods, &super->methods);
		lit_resolver_fields_add_all(resolver->compiler, &class->fields, &super->fields);
	}

	resolver->class = class;
	lit_classes_set(resolver->compiler, &resolver->classes, name, class);
	push_scope(resolver);

	if (statement->fields != NULL) {
		for (int i = 0; i < statement->fields->count; i++) {
			LitFieldStatement* var = ((LitFieldStatement*) statement->fields->values[i]);
			const char* n = var->name;

			LitResolverField* field = (LitResolverField*) reallocate(resolver->compiler, NULL, 0, sizeof(LitResolverField));

			field->access = var->access;
			field->is_static = var->is_static;
			field->is_final = var->final;
			field->original = class;

			resolve_field_statement(resolver, var); // The var->type might be assigned here
			field->type = var->type; // Thats why this must go here

			LitString* name = lit_copy_string(resolver->compiler, n, strlen(n));
			LitResolverField* check_field =	lit_resolver_fields_get(field->is_static ? &class->fields : &class->static_fields, name);

			if (check_field != NULL) {
				error(resolver, statement->statement.line, "Field %s is already defined!", name->chars);
			} else {
				LitResolverMethod* check_method = lit_resolver_methods_get(&class->methods, name);

				if (check_method != NULL) {
					error(resolver, statement->statement.line, "Can't define field and method with the same name %s", name->chars);
				} else {
					check_method = lit_resolver_methods_get(&class->static_methods, name);

					if (check_method != NULL) {
						error(resolver, statement->statement.line, "Can't define field and method with the same name %s", name->chars);
					}
				}
			}

			lit_resolver_fields_set(resolver->compiler, field->is_static ? &class->static_fields : &class->fields, name, field);
		}
	}

	if (statement->methods != NULL) {
		for (int i = 0; i < statement->methods->count; i++) {
			LitMethodStatement* method = statement->methods->values[i];
			char* signature = (char*) get_function_signature(resolver, method->parameters, &method->return_type);

			resolve_method_statement(resolver, method, signature);

			LitResolverMethod* m = (LitResolverMethod*) reallocate(resolver->compiler, NULL, 0, sizeof(LitResolverMethod));

			m->signature = signature;
			m->is_static = method->is_static;
			m->access = method->access;
			m->abstract = method->abstract;
			m->is_overriden = method->overriden;
			m->original = class;

			LitString* name = lit_copy_string(resolver->compiler, method->name, strlen(method->name));
			LitResolverMethod* check_method = lit_resolver_methods_get(method->is_static ? &class->methods : &class->static_methods, name);

			if (check_method != NULL) {
				error(resolver, statement->statement.line, "Method %s is already defined!", name->chars);
			} else {
				LitResolverField* check_field = lit_resolver_fields_get(&class->fields, name);

				if (check_field != NULL) {
					error(resolver, statement->statement.line, "Can't define field and method with the same name %s", name->chars);
				} else {
					check_field = lit_resolver_fields_get(&class->static_fields, name);

					if (check_field != NULL) {
						error(resolver, statement->statement.line, "Can't define field and method with the same name %s", name->chars);
					}
				}
			}

			lit_resolver_methods_set(resolver->compiler, method->is_static ? &class->static_methods : &class->methods, lit_copy_string(resolver->compiler, method->name, strlen(method->name)), m);
		}
	}

	pop_scope(resolver);
	resolver->class = NULL;
	lit_strings_write(resolver->compiler, &resolver->allocated_strings, type);

	if (super != NULL) {
		for (int i = 0; i <= super->methods.capacity_mask; i++) {
			LitResolverMethod* method = super->methods.entries[i].value;

			if (method != NULL) {
				LitResolverMethod* child = lit_resolver_methods_get(&class->methods, super->methods.entries[i].key);

				if (method != child && !child->is_overriden) {
					if (method->abstract) {
						error(resolver, statement->statement.line, "Abstract method %s must be implemented in child class %s", super->methods.entries[i].key->chars, class->name->chars);
					}

					error(resolver, statement->statement.line, "Must use override keyword to override a method");
				}
			}
		}
	}
}

static void resolve_break_statement(LitResolver* resolver, LitBreakStatement* statement) {
	if (resolver->loop == NULL) {
		error(resolver, statement->statement.line, "Can't use 'break' outside of a loop");
	}
}

static void resolve_continue_statement(LitResolver* resolver, LitContinueStatement* statement) {
	if (resolver->loop == NULL) {
		error(resolver, statement->statement.line, "Can't use 'continue' outside of a loop");
	}
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
		case FIELD_STATEMENT:
		case METHOD_STATEMENT: {
			printf("Field or method statement never should be resolved with resolve_statement\n");
			UNREACHABLE();
		}
		case BREAK_STATEMENT: resolve_break_statement(resolver, (LitBreakStatement*) statement); break;
		case CONTINUE_STATEMENT: resolve_continue_statement(resolver, (LitContinueStatement*) statement); break;
		default: {
			printf("Unknown statement with id %i!\n", statement->type);
			UNREACHABLE();
		}
	}
}

static void resolve_statements(LitResolver* resolver, LitStatements* statements) {
	for (int i = 0; i < statements->count; i++) {
		resolve_statement(resolver, statements->values[i]);
	}
}

int strcmp_ignoring(const char* s1, const char* s2) {
	while(*s1 && *s1 != '<' && *s2 != '<' && *s1 == *s2) {
		s1++;
		s2++;
	}

	return *s1 > *s2 ? 1 : (*s1 == *s2 ? 0 : -1);
}

static const char* resolve_binary_expression(LitResolver* resolver, LitBinaryExpression* expression) {
	const char* a = resolve_expression(resolver, expression->left);
	char* b = (char*) resolve_expression(resolver, expression->right);

	if (expression->operator == TOKEN_IS) {
		LitType* class = NULL;

		if (strcmp_ignoring(b, "Class<") == 0) {
			int len = (int) strlen(b);

			// Hack that allows us not to reallocate another string
			b[len - 1] = '\0';
			class = lit_classes_get(&resolver->classes, lit_copy_string(resolver->compiler, &b[6], (size_t) (len - 7)));
			b[len - 1] = '>';
		}

		if (class == NULL) {
			error(resolver, expression->expression.line, "%s is not defined", b);
		}

		return "bool";
	} else {
		if (!((strcmp(a, "int") == 0 || strcmp(a, "double") == 0) && (strcmp(b, "int") == 0 || strcmp(b, "double") == 0))) {
			error(resolver, expression->expression.line, "Can't perform binary operation on %s and %s", a, b);
		}

		return a;
	}
}

static const char* resolve_literal_expression(LitLiteralExpression* expression) {
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
		error(resolver, expression->expression.line, "Can't negate non-number values");
		return "error";
	}

	return type;
}

static const char* resolve_grouping_expression(LitResolver* resolver, LitGroupingExpression* expression) {
	return resolve_expression(resolver, expression->expr);
}

static const char* resolve_var_expression(LitResolver* resolver, LitVarExpression* expression) {
	LitResolverLocal* value = lit_resolver_locals_get(peek_scope(resolver), lit_copy_string(resolver->compiler, expression->name, (int) strlen(expression->name)));

	if (value != NULL && !value->defined) {
		error(resolver, expression->expression.line, "Can't use local variable %s in it's own initializer", expression->name);
		return "error";
	}

	LitResolverLocal* local = resolve_local(resolver, expression->name, expression->expression.line);
	
	if (local == NULL) {
		return "error";
	} else if (local->field && resolver->class != NULL && resolver->depth > 2) {
		error(resolver, expression->expression.line, "Can't access class field %s without this", expression->name);
		return "error";
	}

	return local->type;
}

static const char* resolve_assign_expression(LitResolver* resolver, LitAssignExpression* expression) {
	const char* given = resolve_expression(resolver, expression->value);
	const char* type = resolve_expression(resolver, expression->to);

	if (!compare_arg((char*) type, (char*) given)) {
		error(resolver, expression->expression.line, "Can't assign %s value to a %s var", given, type);
	}

	LitVarExpression* expr = (LitVarExpression*) expression->to;
	LitResolverLocal* local = resolve_local(resolver, expr->name, expression->expression.line);

	if (local == NULL) {
		return "error";
	}

	if (local->final) {
		error(resolver, expression->expression.line, "Can't assign value to a final %s var", type);
	}

	return local->type;
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
	int t = expression->callee->type;

	if (t != VAR_EXPRESSION && t != GET_EXPRESSION && t != GROUPING_EXPRESSION
		&& t != SET_EXPRESSION && t != SUPER_EXPRESSION && t != LAMBDA_EXPRESSION) {

		error(resolver, expression->expression.line, "Can't call non-variable of type %i", t);
	} else {
		const char* type = resolve_expression(resolver, expression->callee);
		const char* name = extract_callee_name(expression->callee);

		if (strcmp_ignoring(type, "Class<") == 0) {
			size_t len = strlen(type);
			return_type = (char*) reallocate(resolver->compiler, NULL, 0, len - 6);
			strncpy(return_type, &type[6], len - 7);
			return_type[len - 7] = '\0';

			lit_strings_write(resolver->compiler, &resolver->allocated_strings, return_type);
			LitType* cl = lit_classes_get(&resolver->classes, lit_copy_string(resolver->compiler, return_type, len - 7));

			if (cl->is_static) {
				error(resolver, expression->expression.line, "Can not create an instance of a static class %s", cl->name->chars);
			} else if (cl->abstract) {
				error(resolver, expression->expression.line, "Can not create an instance of an abstract class %s", cl->name->chars);
			}
		} else if (strcmp_ignoring(type, "Function<") != 0) {
			error(resolver, expression->expression.line, "Can't call non-function variable %s with type %s", name, type);
		} else {
			if (strcmp(type, "error") == 0) {
				error(resolver, expression->expression.line, "Can't call non-defined function %s", name);
				return "error";
			}

			size_t len = strlen(type);
			char* tp = (char*) reallocate(resolver->compiler, NULL, 0, len + 1);
			strncpy(tp, type, len);
			tp[len] = '\0';

			char* arg = tok(&tp[9]);
			int i = 0;
			int cn = expression->args->count;

			while (arg != NULL) {
				if (!had_template && i >= cn) {
					error(resolver, expression->expression.line, "Not enough arguments for %s, expected %i, got %i, for function %s", type, i + 1, cn, name);
					break;
				}

				if (!had_template) {
					const char* given_type = resolve_expression(resolver, expression->args->values[i]);

					if (given_type == NULL) {
						error(resolver, expression->expression.line, "Got null type resolved somehow");
					} else if (!compare_arg(arg, (char*) given_type)) {
						error(resolver, expression->expression.line, "Argument #%i type mismatch: required %s, but got %s, for function %s", i + 1, arg, given_type, name);
					}
				} else {
					size_t l = strlen(arg);
					return_type = (char*) reallocate(resolver->compiler, NULL, 0, l + 1);
					strncpy(return_type, arg, l);
					return_type[l] = '\0';

					lit_strings_write(resolver->compiler, &resolver->allocated_strings, return_type);

					break;
				}

				arg = tok(NULL);
				i++;

				if (arg == NULL) {
					break;
				}
			}

			reallocate(resolver->compiler, tp, len, 0);

			if (i < cn) {
				error(resolver, expression->expression.line, "Too many arguments for function %s, expected %i, got %i, for function %s", type, i, cn, ((LitVarExpression*) expression->callee)->name);
			}
		}
	}

	return return_type;
}

static const char* resolve_get_expression(LitResolver* resolver, LitGetExpression* expression) {
	char* type = (char*) resolve_expression(resolver, expression->object);
	LitType* class = NULL;
	bool should_be_static = false;

	if (strcmp_ignoring(type, "Class<") == 0) {
		int len = (int) strlen(type);

		// Hack that allows us not to reallocate another string
		type[len - 1] = '\0';
		class = lit_classes_get(&resolver->classes, lit_copy_string(resolver->compiler, &type[6], (size_t) (len - 7)));
		type[len - 1] = '>';
		should_be_static = true;
	} else {
		class = lit_classes_get(&resolver->classes, lit_copy_string(resolver->compiler, type, strlen(type)));
	}

	if (class == NULL) {
		error(resolver, expression->expression.line, "Can't find class %s", type);
		return "error";
	} else if (should_be_static && !class->inited) {
		class->inited = true;
		expression->emit_static_init = true;
	}

	LitString* str = lit_copy_string(resolver->compiler, expression->property, strlen(expression->property));

	if (str == resolver->compiler->init_string) {
		error(resolver, expression->expression.line, "Can't call class constructor directly");
	}

	LitResolverField* field = lit_resolver_fields_get(should_be_static ? &class->static_fields : &class->fields, str);

	if (field == NULL) {
		LitResolverMethod* method = lit_resolver_methods_get(should_be_static ? &class->static_methods : &class->methods, str);

		if (method == NULL) {
			error(resolver, expression->expression.line, "%s%s has no %sfield or method %s", should_be_static ? "" : "Class ", type, should_be_static ? "static " : "", expression->property);
			return "error";
		}

		if (should_be_static && !method->is_static) {
			error(resolver, expression->expression.line, "Can't access non-static methods from class call");
		}

		if (method->access == PRIVATE_ACCESS) {
			if (expression->object->type != THIS_EXPRESSION || class->super != NULL) {
				if (expression->object->type != THIS_EXPRESSION || lit_resolver_methods_get(&class->super->methods, str) != NULL || lit_resolver_methods_get(&class->super->static_methods, str) != NULL) {
					error(resolver, expression->expression.line, "Can't access private method %s from %s", expression->property, type);
				}
			}
		} else if (method->access == PROTECTED_ACCESS && expression->object->type != THIS_EXPRESSION && expression->object->type != SUPER_EXPRESSION) {
			error(resolver, expression->expression.line, "Can't access protected method %s", expression->property);
		}

		return method->signature;
	} else if (should_be_static && !field->is_static) {
		error(resolver, expression->expression.line, "Can't access non-static fields from class call");
	}

	return field->type;
}

static const char* resolve_set_expression(LitResolver* resolver, LitSetExpression* expression) {
	const char* type = resolve_expression(resolver, expression->object);

	if (strcmp_ignoring("Class<", type) == 0) {
		size_t len = strlen(type);
		char* tp = (char*) reallocate(resolver->compiler, NULL, 0, len - 6);
		strncpy(tp, &type[6], len - 7);
		tp[len - 7] = '\0';

		lit_strings_write(resolver->compiler, &resolver->allocated_strings, tp);
		LitType* class = lit_classes_get(&resolver->classes, lit_copy_string(resolver->compiler, tp, len - 7));

		if (class == NULL) {
			error(resolver, expression->expression.line, "Undefined type %s", type);
			return "error";
		}

		if (!class->inited) {
			class->inited = true;
			expression->emit_static_init = true;
		}

		LitResolverField *field = lit_resolver_fields_get(&class->static_fields, lit_copy_string(resolver->compiler, expression->property, strlen(expression->property)));

		if (field == NULL) {
			error(resolver, expression->expression.line, "Class %s has no field %s", type, expression->property);
			return "error";
		}

		const char *var_type = expression->value == NULL ? "void" : resolve_expression(resolver, expression->value);

		if (!compare_arg((char *) field->type, (char *) var_type)) {
			error(resolver, expression->expression.line, "Can't assign %s value to %s field %s", var_type, field->type, expression->property);
			return "error";
		}

		if (field->is_final) {
			error(resolver, expression->expression.line, "Field %s is final, can't assign a value to it", expression->property);
		}

		return field->type;
	} else {
		LitType *class = lit_classes_get(&resolver->classes, lit_copy_string(resolver->compiler, type, strlen(type)));

		if (class == NULL) {
			error(resolver, expression->expression.line, "Undefined type %s", type);
			return "error";
		}

		LitResolverField *field = lit_resolver_fields_get(&class->fields, lit_copy_string(resolver->compiler, expression->property, strlen(expression->property)));

		if (field == NULL) {
			error(resolver, expression->expression.line, "Class %s has no field %s", type, expression->property);
			return "error";
		}

		const char *var_type = expression->value == NULL ? "void" : resolve_expression(resolver, expression->value);

		if (!compare_arg((char *) field->type, (char *) var_type)) {
			error(resolver, expression->expression.line, "Can't assign %s value to %s field %s", var_type, field->type, expression->property);
			return "error";
		}

		if (field->is_final) {
			error(resolver, expression->expression.line, "Field %s is final, can't assign a value to it", expression->property);
		}

		return field->type;
	}
}

static const char* resolve_lambda_expression(LitResolver* resolver, LitLambdaExpression* expression) {
	const char* type = get_function_signature(resolver, expression->parameters, &expression->return_type);
	LitFunctionStatement* last = resolver->function;

	resolver->function = (LitFunctionStatement*) expression; // HACKZ
	resolve_function(resolver, expression->parameters, &expression->return_type, expression->body, "Missing return statement in lambda", NULL, expression->expression.line);
	resolver->function = last;

	lit_strings_write(resolver->compiler, &resolver->allocated_strings, (char*) type);

	return type;
}

static const char* resolve_this_expression(LitResolver* resolver, LitThisExpression* expression) {
	if (resolver->class == NULL) {
		error(resolver, expression->expression.line, "Can't use this outside of a class");
		return "error";
	}

	return resolver->class->name->chars;
}

static const char* resolve_super_expression(LitResolver* resolver, LitSuperExpression* expression) {
	if (resolver->class == NULL) {
		error(resolver, expression->expression.line, "Can't use super outside of a class");
		return "error";
	}

	if (resolver->class->super == NULL) {
		error(resolver, expression->expression.line, "Class %s has no super", resolver->class->name->chars);
		return "error";
	}

	LitResolverMethod* method = lit_resolver_methods_get(&resolver->class->super->methods, lit_copy_string(resolver->compiler, expression->method, strlen(expression->method)));

	if (method == NULL) {
		error(resolver, expression->expression.line, "Class %s has no method %s", resolver->class->super->name->chars, expression->method);
		return "error";
	}

	return method->signature;
}

static const char* resolve_if_expression(LitResolver* resolver, LitIfExpression* expression) {
	resolve_expression(resolver, expression->condition);
	const char* type = resolve_expression(resolver, expression->if_branch);

	if (expression->else_if_branches != NULL) {
		resolve_expressions(resolver, expression->else_if_conditions);

		for (int i = 0; i < expression->else_if_branches->count; i++) {
			const char* branch_type = resolve_expression(resolver, expression->else_if_branches->values[i]);

			if (strcmp(branch_type, type) != 0) {
				error(resolver, "If expression type was declared %s (from if branch), can't return a %s value from else if branch", type, branch_type);
			}
		}
	}

	if (expression->else_branch != NULL) {
		const char* branch_type = resolve_expression(resolver, expression->else_branch);

		if (strcmp(branch_type, type) != 0) {
			error(resolver, "If expression type was declared %s (from if branch), can't return a %s value from else branch", type, branch_type);
		}
	}

	return type;
}

static const char* resolve_expression(LitResolver* resolver, LitExpression* expression) {
	switch (expression->type) {
		case BINARY_EXPRESSION: return resolve_binary_expression(resolver, (LitBinaryExpression*) expression);
		case LITERAL_EXPRESSION: return resolve_literal_expression((LitLiteralExpression*) expression);
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
		case IF_EXPRESSION: return resolve_if_expression(resolver, (LitIfExpression*) expression);
		default: {
			printf("Unknown expression with id %i!\n", expression->type);
			UNREACHABLE();
		}
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

	resolver->loop = NULL;
	resolver->had_error = false;
	resolver->depth = 0;
	resolver->function = NULL;
	resolver->class = NULL;

	push_scope(resolver); // Global scope

	lit_define_type(resolver, "error");
	lit_define_type(resolver, "void");
	lit_define_type(resolver, "any");
}

void lit_free_resolver(LitResolver* resolver) {
	pop_scope(resolver);

	for (int i = 0; i <= resolver->externals.capacity_mask; i++) {
		LitResolverLocal* local = resolver->externals.entries[i].value;

		if (local != NULL) {
			reallocate(resolver->compiler, (void*) local->type, strlen(local->type), 0);
			reallocate(resolver->compiler, (void*) local, sizeof(LitResolverLocal), 0);
		}
	}

	lit_free_resolver_locals(resolver->compiler, &resolver->externals);

	for (int i = 0; i <= resolver->classes.capacity_mask; i++) {
		LitType* type = resolver->classes.entries[i].value;

		if (type != NULL && !type->external) {
			if (type->fields.count != 0) {
				for (int j = 0; j <= type->fields.capacity_mask; j++) {
					LitResolverField *a = type->fields.entries[j].value;

					if (a != NULL && a->original == type) {
						reallocate(resolver->compiler, (void *) a, sizeof(LitResolverField), 0);
					}
				}
			}

			if (type->static_fields.count != 0) {
				for (int j = 0; j <= type->static_fields.capacity_mask; j++) {
					LitResolverField *a = type->static_fields.entries[j].value;

					if (a != NULL) {
						reallocate(resolver->compiler, (void *) a, sizeof(LitResolverField), 0);
					}
				}
			}

			if (type->methods.count != 0) {
				for (int j = 0; j <= type->methods.capacity_mask; j++) {
					LitResolverMethod *a = type->methods.entries[j].value;

					if (a != NULL && a->original == type) {
						lit_free_resolver_method(resolver->compiler, a);
					}
				}
			}

			if (type->static_methods.count != 0) {
				for (int j = 0; j <= type->static_methods.capacity_mask; j++) {
					LitResolverMethod *a = type->static_methods.entries[j].value;

					if (a != NULL) {
						lit_free_resolver_method(resolver->compiler, a);
					}
				}
			}

			lit_free_resolver_fields(resolver->compiler, &type->fields);
			lit_free_resolver_fields(resolver->compiler, &type->static_fields);
			lit_free_resolver_methods(resolver->compiler, &type->methods);
			lit_free_resolver_methods(resolver->compiler, &type->static_methods);

			reallocate(resolver->compiler, (void*) type, sizeof(LitType), 0);
		}
	}

	lit_free_classes(resolver->compiler, &resolver->classes);

	for (int i = 0; i < resolver->allocated_strings.count; i++) {
		char* str = resolver->allocated_strings.values[i];
		reallocate(resolver->compiler, (void*) str, strlen(str) + 1, 0);
	}

	lit_free_strings(resolver->compiler, &resolver->allocated_strings);
	lit_free_types(resolver->compiler, &resolver->types);
	lit_free_scopes(resolver->compiler, &resolver->scopes);
}

bool lit_resolve(LitCompiler* compiler, LitStatements* statements) {
	resolve_statements(&compiler->resolver, statements);
	return compiler->resolver.had_error;
}

void lit_free_resolver_field(LitCompiler* compiler, LitResolverField* field) {
	// Nothing to free :P
}

void lit_free_resolver_method(LitCompiler* compiler, LitResolverMethod* method) {
	reallocate(compiler, (void*) method->signature, strlen(method->signature) + 1, 0);
	reallocate(compiler, (void*) method, sizeof(LitResolverMethod), 0);
}

void lit_init_type(LitType* type) {
	type->name = NULL;
	type->is_static = false;
	type->final = false;
	type->abstract = false;
	type->inited = false;
	type->super = NULL;

	lit_init_resolver_methods(&type->methods);
	lit_init_resolver_methods(&type->static_methods);

	lit_init_resolver_fields(&type->fields);
	lit_init_resolver_fields(&type->static_fields);
}

void lit_free_type(LitMemManager* manager, LitType* type) {
	lit_free_resolver_methods(manager, &type->methods);
	lit_free_resolver_methods(manager, &type->static_methods);

	lit_free_resolver_fields(manager, &type->fields);
	lit_free_resolver_fields(manager, &type->static_fields);
}