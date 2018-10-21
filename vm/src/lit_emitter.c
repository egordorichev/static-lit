#include <stdio.h>
#include <memory.h>
#include <zconf.h>
#include <lit.h>

#include "lit_emitter.h"
#include "lit_debug.h"
#include "lit_memory.h"

static void emit_byte(LitEmitter* emitter, uint8_t byte) {
	lit_chunk_write(emitter->vm, &emitter->function->function->chunk, byte);
}

static void emit_bytes(LitEmitter* emitter, uint8_t a, uint8_t b) {
	lit_chunk_write(emitter->vm, &emitter->function->function->chunk, a);
	lit_chunk_write(emitter->vm, &emitter->function->function->chunk, b);
}

static void error(LitEmitter* emitter, const char* format, ...) {
	fflush(stdout);

	va_list vargs;
	va_start(vargs, format);
	fprintf(stderr, "Error: ");
	vfprintf(stderr, format, vargs);
	fprintf(stderr, "\n");
	va_end(vargs);

	fflush(stderr);
	emitter->had_error = true;
}

static uint8_t make_constant(LitEmitter* emitter, LitValue value) {
	int constant = lit_chunk_add_constant(emitter->vm, &emitter->function->function->chunk, value);

	if (constant > UINT8_MAX) {
		error(emitter, "Too many constants in one chunk");
		return 0;
	}

	return (uint8_t) constant;
}

static void emit_constant(LitEmitter* emitter, LitValue constant) {
	emit_bytes(emitter, OP_CONSTANT, make_constant(emitter, constant));
}

static int emit_jump(LitEmitter* emitter, uint8_t instruction) {
	emit_byte(emitter, instruction);
	emit_bytes(emitter, 0xff, 0xff);

	return emitter->function->function->chunk.count - 2;
}

static void patch_jump(LitEmitter* emitter, int offset) {
	LitChunk* chunk = &emitter->function->function->chunk;
	int jump = chunk->count - offset - 2;

	if (jump > UINT16_MAX) {
		error(emitter, "Too much code to jump over");
	}

	chunk->code[offset] = (uint8_t) ((jump >> 8) & 0xff);
	chunk->code[offset + 1] = (uint8_t) (jump & 0xff);
}

static void emit_loop(LitEmitter* emitter, int loop_start) {
	emit_byte(emitter, OP_LOOP);
	int offset = emitter->function->function->chunk.count - loop_start + 2;

	if (offset > UINT16_MAX) {
		error(emitter, "Loop body too large");
	}

	emit_bytes(emitter, (uint8_t) ((offset >> 8) & 0xff), (uint8_t) (offset & 0xff));
}

static int resolve_local(LitEmitterFunction* function, const char* name) {
	for (int i = function->local_count - 1; i >= 0; i--) {
		LitLocal* local = &function->locals[i];

		if (strcmp(name, local->name) == 0) {
			return i;
		}
	}

	return -1;
}

static int add_upvalue(LitEmitter* emitter, LitEmitterFunction* function, uint8_t index, bool is_local);
static int add_local(LitEmitter* emitter, const char* name);
static void emit_statement(LitEmitter* emitter, LitStatement* statement);
static bool no_pop;

static int resolve_upvalue(LitEmitter* emitter, LitEmitterFunction* function, char* name) {
	if (function->enclosing == NULL) {
		return -1;
	}

	int local = resolve_local(function->enclosing, name);

	if (local != -1) {
		function->enclosing->locals[local].upvalue = true;
		return add_upvalue(emitter, function, (uint8_t) local, true);
	}

	int upvalue = resolve_upvalue(emitter, function->enclosing, name);

	if (upvalue != -1) {
		return add_upvalue(emitter, function, (uint8_t) upvalue, false);
	}

	return -1;
}

static void emit_expression(LitEmitter* emitter, LitExpression* expression) {
	switch (expression->type) {
		case BINARY_EXPRESSION: {
			LitBinaryExpression* expr = (LitBinaryExpression*) expression;

			emit_expression(emitter, expr->left);
			emit_expression(emitter, expr->right);

			switch (expr->operator) {
				case TOKEN_BANG_EQUAL: emit_byte(emitter, OP_NOT_EQUAL); break;
				case TOKEN_EQUAL_EQUAL: emit_byte(emitter,OP_EQUAL); break;
				case TOKEN_GREATER: emit_byte(emitter, OP_GREATER); break;
				case TOKEN_GREATER_EQUAL: emit_byte(emitter, OP_GREATER_EQUAL); break;
				case TOKEN_LESS: emit_byte(emitter, OP_LESS); break;
				case TOKEN_LESS_EQUAL: emit_byte(emitter, OP_LESS_EQUAL); break;
				case TOKEN_PLUS: emit_byte(emitter, OP_ADD); break;
				case TOKEN_MINUS: emit_byte(emitter, OP_SUBTRACT); break;
				case TOKEN_STAR: emit_byte(emitter, OP_MULTIPLY); break;
				case TOKEN_SLASH: emit_byte(emitter, OP_DIVIDE); break;
			}

			break;
		}
		case LITERAL_EXPRESSION: emit_constant(emitter, ((LitLiteralExpression*) expression)->value); break;
		case UNARY_EXPRESSION: {
			LitUnaryExpression* expr = (LitUnaryExpression*) expression;
			emit_expression(emitter, expr->right);

			switch (expr->operator) {
				case TOKEN_BANG: emit_byte(emitter, OP_NOT); break;
				case TOKEN_MINUS: emit_byte(emitter, OP_NEGATE); break;
			}

			break;
		}
		case GROUPING_EXPRESSION: emit_expression(emitter, ((LitGroupingExpression*) expression)->expr); break;
		case VAR_EXPRESSION: {
			LitVarExpression* expr = (LitVarExpression*) expression;
			int local = resolve_local(emitter->function, expr->name);


			if (local != -1) {
				emit_bytes(emitter, OP_GET_LOCAL, (uint8_t) local);
			} else {
				int upvalue = resolve_upvalue(emitter, emitter->function, (char*) expr->name);

				if (upvalue != -1) {
					emit_bytes(emitter, OP_GET_UPVALUE, (uint8_t) upvalue);
				} else {
					emit_bytes(emitter, OP_GET_GLOBAL, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(emitter->vm, expr->name, strlen(expr->name)))));
				}
			}

			break;
		}
		case ASSIGN_EXPRESSION: {
			LitAssignExpression* expr = (LitAssignExpression*) expression;
			LitVarExpression* e = (LitVarExpression*) expr->to;

			emit_expression(emitter, expr->value);
			int local = resolve_local(emitter->function, e->name);

			if (local != -1) {
				emit_bytes(emitter, OP_SET_LOCAL, (uint8_t) local);
			} else {
				int upvalue = resolve_upvalue(emitter, emitter->function, (char*) e->name);

				if (upvalue != -1) {
					emit_bytes(emitter, OP_SET_UPVALUE, (uint8_t) upvalue);
				} else {
					emit_bytes(emitter, OP_SET_GLOBAL, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(emitter->vm, e->name, strlen(e->name)))));
				}
			}

			break;
		}
		case LOGICAL_EXPRESSION: {
			LitLogicalExpression* expr = (LitLogicalExpression*) expression;

			switch (expr->operator) {
				case TOKEN_OR: {
					emit_expression(emitter, expr->left);

					int else_jump = emit_jump(emitter, OP_JUMP_IF_FALSE);
					int end_jump = emit_jump(emitter, OP_JUMP);

					patch_jump(emitter, else_jump);
					emit_byte(emitter, OP_POP);
					emit_expression(emitter, expr->right);
					patch_jump(emitter, end_jump);

					break;
				}
				case TOKEN_AND: {
					emit_expression(emitter, expr->left);
					int endJump = emit_jump(emitter, OP_JUMP_IF_FALSE);

					emit_byte(emitter, OP_POP);
					emit_expression(emitter, expr->right);
					patch_jump(emitter, endJump);

					break;
				}
			}

			break;
		}
		case CALL_EXPRESSION: {
			LitCallExpression* expr = (LitCallExpression*) expression;
			emit_expression(emitter, expr->callee);

			if (expr->callee->type == GET_EXPRESSION) {
				emit_expression(emitter, ((LitGetExpression*) expr->callee)->object);
			}

			if (expr->args != NULL) {
				for (int i = 0; i < expr->args->count; i++) {
					emit_expression(emitter, expr->args->values[i]);
				}

				emit_constant(emitter, MAKE_NUMBER_VALUE(expr->args->count));
			} else {
				emit_constant(emitter, MAKE_NUMBER_VALUE(0));
			}

			if (expr->callee->type == GET_EXPRESSION) {
				emit_byte(emitter, OP_INVOKE);
			} else {
				emit_byte(emitter, OP_CALL);
			}

			break;
		}
		case GET_EXPRESSION: {
			LitGetExpression* expr = (LitGetExpression*) expression;

			emit_expression(emitter, expr->object);
			emit_bytes(emitter, OP_GET_PROPERTY, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(emitter->vm, expr->property, strlen(expr->property)))));

			break;
		}
		case SET_EXPRESSION: {
			LitSetExpression* expr = (LitSetExpression*) expression;

			emit_expression(emitter, expr->object);
			emit_expression(emitter, expr->value);
			emit_bytes(emitter, OP_SET_PROPERTY, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(emitter->vm, expr->property, strlen(expr->property)))));

			break;
		}
		case LAMBDA_EXPRESSION: {
			LitLambdaExpression* expr = (LitFunctionStatement*) expression;
			LitEmitterFunction function;

			function.function = lit_new_function(emitter->vm);
			function.depth = emitter->function->depth + 1;
			function.local_count = 0;
			function.enclosing = emitter->function;
			function.function = lit_new_function(emitter->vm);
			function.function->name = lit_copy_string(emitter->vm, "lambda", 6);
			function.function->arity = expr->parameters == NULL ? 0 : expr->parameters->count;

			emitter->function = &function;

			// add_local(emitter, "this");

			if (expr->parameters != NULL) {
				for (int i = 0; i < expr->parameters->count; i++) {
					add_local(emitter, expr->parameters->values[i].name);
				}
			}

			emit_statement(emitter, expr->body);

			if (DEBUG_PRINT_CODE) {
				lit_trace_chunk(emitter->vm, &function.function->chunk, "lambda");
			}

			emitter->function = function.enclosing;
			emit_bytes(emitter, OP_CLOSURE, make_constant(emitter, MAKE_OBJECT_VALUE(function.function)));

			for (int i = 0; i < function.function->upvalue_count; i++) {
				emit_byte(emitter, (uint8_t) (function.upvalues[i].local ? 1 : 0));
				emit_byte(emitter, function.upvalues[i].index);
			}

			break;
		}
		case THIS_EXPRESSION: {
			emit_bytes(emitter, OP_GET_LOCAL, 0);
			break;
		}
		case SUPER_EXPRESSION: {
			LitSuperExpression* expr = (LitSuperExpression*) expression;
			emit_bytes(emitter, OP_SUPER, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(emitter->vm, expr->method, strlen(expr->method)))));

			break;
		}
	}
}

static void emit_expressions(LitEmitter* emitter, LitExpressions* expressions) {
	for (int i = 0; i < expressions->count; i++) {
		emit_expression(emitter, expressions->values[i]);
	}
}

static void emit_statements(LitEmitter* emitter, LitStatements* statements);

static int add_upvalue(LitEmitter* emitter, LitEmitterFunction* function, uint8_t index, bool is_local) {
	int upvalue_count = function->function->upvalue_count;

	for (int i = 0; i < upvalue_count; i++) {
		LitEmvalue* upvalue = &function->upvalues[i];

		if (upvalue->index == index && upvalue->local == is_local) {
			return i;
		}
	}

	if (upvalue_count == UINT8_COUNT) {
		error(emitter, "Too many closure variables in function");
		return 0;
	}

	function->upvalues[upvalue_count].local = is_local;
	function->upvalues[upvalue_count].index = index;

	return function->function->upvalue_count++;
}

static int add_local(LitEmitter* emitter, const char* name) {
	if (emitter->function->local_count == UINT8_COUNT) {
		error(emitter, "Too many local variables in function");
		return -1;
	}

	if (strcmp(name, "this") == 0) {
		LitLocal* local = &emitter->function->locals[0];

		local->name = name;
		local->depth = emitter->function->depth;
		local->upvalue = false;

		return 0;
	} else {
		LitLocal* local = &emitter->function->locals[emitter->function->local_count];

		local->name = name;
		local->depth = emitter->function->depth;
		local->upvalue = false;

		emitter->function->local_count++;

		return emitter->function->local_count - 1;
	}
}

static void emit_statement(LitEmitter* emitter, LitStatement* statement) {
	switch (statement->type) {
		case VAR_STATEMENT: {
			LitVarStatement* stmt = (LitVarStatement*) statement;

			if (stmt->init != NULL) {
				emit_expression(emitter, stmt->init);
			} else {
				emit_byte(emitter, OP_NIL); // TODO: default type! (like bool is false)
			}

			if (emitter->function->depth == 0) {
				int str = make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(emitter->vm, stmt->name, strlen(stmt->name))));
				emit_bytes(emitter, OP_DEFINE_GLOBAL, (uint8_t) str);
			} else {
				emit_bytes(emitter, OP_SET_LOCAL, (uint8_t) add_local(emitter, stmt->name));
			}

			break;
		}
		case EXPRESSION_STATEMENT:
			emit_expression(emitter, ((LitExpressionStatement*) statement)->expr);

			if (!no_pop) {
				emit_byte(emitter, OP_POP);
			} else {
				no_pop = false;
			}

			break;
		case IF_STATEMENT: {
			LitIfStatement* stmt = (LitIfStatement*) statement;
			emit_expression(emitter, stmt->condition);

			int else_jump = emit_jump(emitter, OP_JUMP_IF_FALSE);
			emit_statement(emitter, stmt->if_branch);

			int end_jump = emit_jump(emitter, OP_JUMP);
			int end_jumps[stmt->else_if_branches == NULL ? 0 : stmt->else_if_branches->count];

			if (stmt->else_if_branches != NULL) {
				for (int i = 0; i < stmt->else_if_branches->count; i++) {
					patch_jump(emitter, else_jump);
					emit_expression(emitter, stmt->else_if_conditions->values[i]);
					else_jump = emit_jump(emitter, OP_JUMP_IF_FALSE);
					emit_statement(emitter, stmt->else_if_branches->values[i]);

					end_jumps[i] = emit_jump(emitter, OP_JUMP);
				}
			}

			patch_jump(emitter, else_jump);

			if (stmt->else_branch != NULL) {
				emit_statement(emitter, stmt->else_branch);
			}

			if (stmt->else_if_branches != NULL) {
				for (int i = 0; i < stmt->else_if_branches->count; i++) {
					patch_jump(emitter, end_jumps[i]);
				}
			}

			patch_jump(emitter, end_jump);
			break;
		}
		case BLOCK_STATEMENT: {
			LitBlockStatement* stmt = ((LitBlockStatement*) statement);

			if (stmt->statements != NULL) {
				emit_statements(emitter, stmt->statements);
				emit_byte(emitter, OP_POP);
			}

			break;
		}
		case WHILE_STATEMENT: {
			LitWhileStatement* stmt = (LitWhileStatement*) statement;

			int loop_start = emitter->function->function->chunk.count;
			no_pop = true;
			emit_expression(emitter, stmt->condition);
			int exit_jump = emit_jump(emitter, OP_JUMP_IF_FALSE);
			emit_byte(emitter, OP_POP);

			emit_statement(emitter, stmt->body);
			emit_loop(emitter, loop_start);
			patch_jump(emitter, exit_jump);

			break;
		}
		case FUNCTION_STATEMENT: {
			LitFunctionStatement* stmt = (LitFunctionStatement*) statement;
			LitEmitterFunction function;

			function.function = lit_new_function(emitter->vm);
			function.depth = emitter->function->depth + 1;
			function.local_count = 0;
			function.enclosing = emitter->function;
			function.function = lit_new_function(emitter->vm);
			function.function->name = lit_copy_string(emitter->vm, stmt->name, (int) strlen(stmt->name));
			function.function->arity = stmt->parameters == NULL ? 0 : stmt->parameters->count;

			emitter->function = &function;

			if (stmt->parameters != NULL) {
				for (int i = 0; i < stmt->parameters->count; i++) {
					add_local(emitter, stmt->parameters->values[i].name);
				}
			}

			emit_statement(emitter, stmt->body);

			if (DEBUG_PRINT_CODE) {
				lit_trace_chunk(emitter->vm, &function.function->chunk, stmt->name);
			}

			emitter->function = function.enclosing;
			emit_bytes(emitter, OP_CLOSURE, make_constant(emitter, MAKE_OBJECT_VALUE(function.function)));

			for (int i = 0; i < function.function->upvalue_count; i++) {
				emit_byte(emitter, (uint8_t) (function.upvalues[i].local ? 1 : 0));
				emit_byte(emitter, function.upvalues[i].index);
			}

			if (emitter->function->depth == 0) {
				emit_bytes(emitter, OP_DEFINE_GLOBAL, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(emitter->vm, stmt->name, strlen(stmt->name)))));
			} else {
				emit_bytes(emitter, OP_SET_LOCAL, (uint8_t) add_local(emitter, stmt->name));
			}

			break;
		}
		case RETURN_STATEMENT: {
			LitReturnStatement* stmt = (LitReturnStatement*) statement;

			if (stmt->value == NULL) {
				// FIXME: should not emit nil in init() method
				// emit_byte(emitter, OP_NIL);
			} else {
				emit_expression(emitter, stmt->value);
			}

			emit_byte(emitter, OP_RETURN);
			break;
		}
		case CLASS_STATEMENT: {
			LitClassStatement* stmt = (LitClassStatement*) statement;

			if (stmt->super != NULL) {
				emit_expression(emitter, (LitExpression*) stmt->super);
				emit_bytes(emitter, OP_SUBCLASS, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(emitter->vm, stmt->name, strlen(stmt->name)))));
			} else {
				emit_bytes(emitter, OP_CLASS, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(emitter->vm, stmt->name, strlen(stmt->name)))));
			}

			if (stmt->fields != NULL) {
				for (int i = 0; i < stmt->fields->count; i++) {
					LitVarStatement* field = (LitVarStatement*) stmt->fields->values[i];

					if (field->init != NULL) {
						emit_expression(emitter, field->init);
					} else {
						emit_byte(emitter, OP_NIL); // TODO: default type! (like bool is false), also, shouldn't allow something like var a, should specify type
					}

					emit_bytes(emitter, OP_DEFINE_PROPERTY, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(emitter->vm, field->name, strlen(field->name)))));
				}
			}

			if (stmt->methods != NULL) {
				for (int j = 0; j < stmt->methods->count; j++) {
					LitFunctionStatement* method = stmt->methods->values[j];
					LitEmitterFunction function;

					function.function = lit_new_function(emitter->vm);
					function.depth = emitter->function->depth + 1;
					function.local_count = 1;
					function.enclosing = emitter->function;
					function.function = lit_new_function(emitter->vm);

					size_t name_len = strlen(method->name);
					size_t type_len = strlen(stmt->name);

					char* name = (char*) reallocate(emitter->vm, NULL, 0, name_len + type_len + 1);

					strncpy(name, stmt->name, type_len);
					name[type_len] = '.';
					strncpy(&name[type_len + 1], method->name, name_len);

					function.function->name = lit_copy_string(emitter->vm, name, name_len + type_len + 1);
					function.function->arity = method->parameters == NULL ? 0 : method->parameters->count;

					emitter->function = &function;

					add_local(emitter, "this");

					if (method->parameters != NULL) {
						for (int i = 0; i < method->parameters->count; i++) {
							add_local(emitter, method->parameters->values[i].name);
						}
					}

					emit_statement(emitter, method->body);

					if (DEBUG_PRINT_CODE) {
						lit_trace_chunk(emitter->vm, &function.function->chunk, method->name);
					}

					emitter->function = function.enclosing;
					emit_bytes(emitter, OP_CLOSURE, make_constant(emitter, MAKE_OBJECT_VALUE(function.function)));

					for (int i = 0; i < function.function->upvalue_count; i++) {
						emit_byte(emitter, (uint8_t) (function.upvalues[i].local ? 1 : 0));
						emit_byte(emitter, function.upvalues[i].index);
					}

					emit_bytes(emitter, OP_DEFINE_METHOD, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(emitter->vm, method->name, strlen(method->name)))));
				}
			}

			emit_bytes(emitter, OP_DEFINE_GLOBAL, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(emitter->vm, stmt->name, strlen(stmt->name)))));
			break;
		}
	}
}

static void emit_statements(LitEmitter* emitter, LitStatements* statements) {
	for (int i = 0; i < statements->count; i++) {
		emit_statement(emitter, statements->values[i]);
	}
}


LitFunction* lit_emit(LitVm* vm, LitStatements* statements) {
	LitEmitter emitter;
	LitEmitterFunction function;

	function.function = lit_new_function(vm);
	function.depth = 0;
	function.local_count = 0;
	function.enclosing = NULL;

	emitter.had_error = false;
	emitter.vm = vm;
	emitter.function = &function;
	emitter.class = NULL;

	emit_statements(&emitter, statements);
	emit_byte(&emitter, OP_NIL);
	emit_byte(&emitter, OP_RETURN);

	return emitter.had_error ? NULL : function.function;
}