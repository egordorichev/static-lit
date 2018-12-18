#include <stdio.h>
#include <memory.h>
#include <zconf.h>
#include <lit.h>

#include <lit_debug.h>

#include <compiler/lit_emitter.h>
#include <vm/lit_memory.h>
#include <compiler/lit_ast.h>

DEFINE_ARRAY(LitInts, uint64_t, ints)

static void emit_byte(LitEmitter* emitter, uint8_t byte, uint64_t line) {
	lit_chunk_write(MM(emitter->compiler), &emitter->function->function->chunk, byte, line);
}

static void emit_bytes(LitEmitter* emitter, uint8_t a, uint8_t b, uint64_t line) {
	lit_chunk_write(MM(emitter->compiler), &emitter->function->function->chunk, a, line);
	lit_chunk_write(MM(emitter->compiler), &emitter->function->function->chunk, b, line);
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
	int constant = lit_chunk_add_constant(MM(emitter->compiler), &emitter->function->function->chunk, value);

	if (constant > UINT8_MAX) {
		error(emitter, "Too many constants in one chunk");
		return 0;
	}

	return (uint8_t) constant;
}

static void emit_constant(LitEmitter* emitter, LitValue constant, uint64_t line) {
	emit_bytes(emitter, OP_CONSTANT, make_constant(emitter, constant), line);
}

static uint64_t emit_jump(LitEmitter* emitter, uint8_t instruction, uint64_t line) {
	emit_byte(emitter, instruction, line);
	emit_bytes(emitter, 0xff, 0xff, line);

	return emitter->function->function->chunk.count - 2;
}

static uint64_t emit_jump_instant(LitEmitter* emitter, uint8_t instruction, uint64_t jump, uint64_t line) {
	emit_byte(emitter, instruction, line);
	emit_bytes(emitter, (uint8_t) ((jump >> 8) & 0xff), (uint8_t) (jump & 0xff), line);

	return emitter->function->function->chunk.count - 2;
}

static void patch_jump(LitEmitter* emitter, uint64_t offset) {
	LitChunk* chunk = &emitter->function->function->chunk;
	uint64_t jump = chunk->count - offset - 2;

	if (jump > UINT16_MAX) {
		// FIXME: introduce OP_LONG_JUMP
		error(emitter, "Too much code to jump over");
	}

	chunk->code[offset] = (uint8_t) ((jump >> 8) & 0xff);
	chunk->code[offset + 1] = (uint8_t) (jump & 0xff);
}

static void emit_loop(LitEmitter* emitter, uint64_t loop_start, uint64_t line) {
	emit_byte(emitter, OP_LOOP, line);
	uint64_t offset = emitter->function->function->chunk.count - loop_start + 2;

	if (offset > UINT16_MAX) {
		error(emitter, "Loop body too large");
	}

	emit_bytes(emitter, (uint8_t) ((offset >> 8) & 0xff), (uint8_t) (offset & 0xff), line);
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
				case TOKEN_BANG_EQUAL: emit_byte(emitter, OP_NOT_EQUAL, expression->line); break;
				case TOKEN_EQUAL_EQUAL: emit_byte(emitter, OP_EQUAL, expression->line); break;
				case TOKEN_GREATER: emit_byte(emitter, OP_GREATER, expression->line); break;
				case TOKEN_GREATER_EQUAL: emit_byte(emitter, OP_GREATER_EQUAL, expression->line); break;
				case TOKEN_LESS: emit_byte(emitter, OP_LESS, expression->line); break;
				case TOKEN_LESS_EQUAL: emit_byte(emitter, OP_LESS_EQUAL, expression->line); break;
				case TOKEN_PLUS: emit_byte(emitter, OP_ADD, expression->line); break;
				case TOKEN_MINUS: emit_byte(emitter, OP_SUBTRACT, expression->line); break;
				case TOKEN_STAR: emit_byte(emitter, OP_MULTIPLY, expression->line); break;
				case TOKEN_SLASH: emit_byte(emitter, OP_DIVIDE, expression->line); break;
				case TOKEN_CARET: emit_byte(emitter, OP_POWER, expression->line); break;
				case TOKEN_CELL: emit_byte(emitter, OP_ROOT, expression->line); break;
				case TOKEN_IS: emit_byte(emitter, OP_IS, expression->line); break;
				case TOKEN_PERCENT: emit_byte(emitter, OP_MODULO, expression->line); break;
				default: UNREACHABLE();
			}

			break;
		}
		case LITERAL_EXPRESSION: {
			emit_constant(emitter, ((LitLiteralExpression*) expression)->value, expression->line);
			break;
		}
		case UNARY_EXPRESSION: {
			LitUnaryExpression* expr = (LitUnaryExpression*) expression;
			emit_expression(emitter, expr->right);

			switch (expr->operator) {
				case TOKEN_BANG: emit_byte(emitter, OP_NOT, expression->line); break;
				case TOKEN_MINUS: emit_byte(emitter, OP_NEGATE, expression->line); break;
				case TOKEN_CELL: emit_byte(emitter, OP_SQUARE, expression->line); break;
				default: UNREACHABLE();
			}

			break;
		}
		case GROUPING_EXPRESSION: {
			emit_expression(emitter, ((LitGroupingExpression*) expression)->expr);
			break;
		}
		case VAR_EXPRESSION: {
			LitVarExpression* expr = (LitVarExpression*) expression;
			int local = resolve_local(emitter->function, expr->name);

			if (local != -1) {
				emit_bytes(emitter, OP_GET_LOCAL, (uint8_t) local, expression->line);
			} else {
				int upvalue = resolve_upvalue(emitter, emitter->function, (char*) expr->name);

				if (upvalue != -1) {
					emit_bytes(emitter, OP_GET_UPVALUE, (uint8_t) upvalue, expression->line);
				} else {
					emit_bytes(emitter, OP_GET_GLOBAL, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(MM(emitter->compiler), expr->name, strlen(expr->name)))), expression->line);
				}
			}

			break;
		}
		case ASSIGN_EXPRESSION: {
			LitAssignExpression* expr = (LitAssignExpression*) expression;

			if (expr->to->type == GET_EXPRESSION) {
				LitGetExpression* e = (LitGetExpression*) expr->to;

				emit_expression(emitter, e->object);
				emit_expression(emitter, expr->value);

				emit_bytes(emitter, OP_SET_FIELD, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(MM(emitter->compiler), e->property, strlen(e->property)))), expression->line);
			} else {
				LitVarExpression *e = (LitVarExpression*) expr->to;

				emit_expression(emitter, expr->value);
				int local = resolve_local(emitter->function, e->name);

				if (local != -1) {
					emit_bytes(emitter, OP_SET_LOCAL, (uint8_t) local, expression->line);
				} else {
					int upvalue = resolve_upvalue(emitter, emitter->function, (char *) e->name);

					if (upvalue != -1) {
						emit_bytes(emitter, OP_SET_UPVALUE, (uint8_t) upvalue, expression->line);
					} else {
						emit_bytes(emitter, OP_SET_GLOBAL, make_constant(emitter, MAKE_OBJECT_VALUE(
							lit_copy_string(MM(emitter->compiler), e->name, strlen(e->name)))), expression->line);
					}
				}
			}

			break;
		}
		case LOGICAL_EXPRESSION: {
			LitLogicalExpression* expr = (LitLogicalExpression*) expression;

			switch (expr->operator) {
				case TOKEN_OR: {
					emit_expression(emitter, expr->left);

					uint64_t else_jump = emit_jump(emitter, OP_JUMP_IF_FALSE, expression->line);
					uint64_t end_jump = emit_jump(emitter, OP_JUMP, expression->line);

					patch_jump(emitter, else_jump);
					emit_byte(emitter, OP_POP, expression->line);
					emit_expression(emitter, expr->right);
					patch_jump(emitter, end_jump);

					break;
				}
				case TOKEN_AND: {
					emit_expression(emitter, expr->left);
					uint64_t endJump = emit_jump(emitter, OP_JUMP_IF_FALSE, expression->line);

					emit_byte(emitter, OP_POP, expression->line);
					emit_expression(emitter, expr->right);
					patch_jump(emitter, endJump);

					break;
				}
				default: UNREACHABLE();
			}

			break;
		}
		case CALL_EXPRESSION: {
			LitCallExpression* expr = (LitCallExpression*) expression;

			if (expr->callee->type == GET_EXPRESSION) {
				emit_expression(emitter, ((LitGetExpression*) expr->callee)->object);
			}

			emit_expression(emitter, expr->callee);

			if (expr->args != NULL) {
				for (int i = 0; i < expr->args->count; i++) {
					emit_expression(emitter, expr->args->values[i]);
				}
			}

			if (expr->callee->type == GET_EXPRESSION) {
				emit_byte(emitter, OP_INVOKE, expression->line);
			} else {
				emit_byte(emitter, OP_CALL, expression->line);
			}

			if (expr->args != NULL) {
				emit_byte(emitter, (uint8_t) expr->args->count, expression->line);
			} else {
				emit_byte(emitter, 0, expression->line);
			}

			break;
		}
		case GET_EXPRESSION: {
			LitGetExpression* expr = (LitGetExpression*) expression;

			if (expr->emit_static_init) {
				emit_byte(emitter, OP_STATIC_INIT, expression->line);
			}

			emit_expression(emitter, expr->object);
			emit_bytes(emitter, OP_GET_FIELD, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(MM(emitter->compiler), expr->property, strlen(expr->property)))), expression->line);

			break;
		}
		case SET_EXPRESSION: {
			LitSetExpression* expr = (LitSetExpression*) expression;

			emit_expression(emitter, expr->object);

			if (expr->emit_static_init) {
				emit_byte(emitter, OP_STATIC_INIT, expression->line);
			}

			emit_expression(emitter, expr->value);
			emit_bytes(emitter, OP_SET_FIELD, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(MM(emitter->compiler), expr->property, strlen(expr->property)))), expression->line);

			break;
		}
		case LAMBDA_EXPRESSION: {
			LitLambdaExpression* expr = (LitLambdaExpression*) expression;
			LitEmitterFunction function;

			function.function = lit_new_function(MM(emitter->compiler));
			function.depth = emitter->function->depth + 1;
			function.local_count = 0;
			function.enclosing = emitter->function;
			function.function = lit_new_function(MM(emitter->compiler));
			function.function->name = lit_copy_string(MM(emitter->compiler), "lambda", 6);
			function.function->arity = expr->parameters == NULL ? 0 : expr->parameters->count;

			emitter->function = &function;

			// add_local(emitter, "this"); TODO: might refer to self? *why would you need that, tho?

			if (expr->parameters != NULL) {
				for (int i = 0; i < expr->parameters->count; i++) {
					add_local(emitter, expr->parameters->values[i].name);
				}
			}

			emit_statement(emitter, expr->body);

			if (DEBUG_TRACE_CODE) {
				lit_trace_chunk(MM(emitter->compiler), &function.function->chunk, "lambda");
			}

			emitter->function = function.enclosing;
			emit_bytes(emitter, OP_CLOSURE, make_constant(emitter, MAKE_OBJECT_VALUE(function.function)), expression->line);

			for (int i = 0; i < function.function->upvalue_count; i++) {
				emit_byte(emitter, (uint8_t) (function.upvalues[i].local ? 1 : 0), expression->line);
				emit_byte(emitter, function.upvalues[i].index, expression->line);
			}

			break;
		}
		case THIS_EXPRESSION: {
			emit_bytes(emitter, OP_GET_LOCAL, 0, expression->line);
			break;
		}
		case SUPER_EXPRESSION: {
			LitSuperExpression* expr = (LitSuperExpression*) expression;
			emit_bytes(emitter, OP_SUPER, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(MM(emitter->compiler), expr->method, strlen(expr->method)))), expression->line);

			break;
		}
		case IF_EXPRESSION: {
			LitIfExpression* expr = (LitIfExpression*) expression;
			emit_expression(emitter, expr->condition);

			uint64_t else_jump = emit_jump(emitter, OP_JUMP_IF_FALSE, expression->line);
			emit_byte(emitter, OP_POP, expression->line);
			emit_expression(emitter, expr->if_branch);

			uint64_t end_jump = emit_jump(emitter, OP_JUMP, expression->line);
			uint64_t end_jumps[expr->else_if_branches == NULL ? 0 : expr->else_if_branches->count];

			if (expr->else_if_branches != NULL) {
				for (int i = 0; i < expr->else_if_branches->count; i++) {
					patch_jump(emitter, else_jump);
					emit_byte(emitter, OP_POP, expression->line);
					emit_expression(emitter, expr->else_if_conditions->values[i]);
					else_jump = emit_jump(emitter, OP_JUMP_IF_FALSE, expression->line);
					emit_byte(emitter, OP_POP, expression->line);
					emit_expression(emitter, expr->else_if_branches->values[i]);

					end_jumps[i] = emit_jump(emitter, OP_JUMP, expression->line);
				}
			}

			patch_jump(emitter, else_jump);
			emit_byte(emitter, OP_POP, expression->line);

			if (expr->else_branch != NULL) {
				emit_expression(emitter, expr->else_branch);
			}

			if (expr->else_if_branches != NULL) {
				for (int i = 0; i < expr->else_if_branches->count; i++) {
					patch_jump(emitter, end_jumps[i]);
				}
			}

			patch_jump(emitter, end_jump);
			break;
		}
		default: {
			printf("Unknown expression with id %i!\n", expression->type);
			UNREACHABLE();
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
				if (stmt->default_value == OP_CONSTANT) {
					emit_constant(emitter, MAKE_NUMBER_VALUE(0), statement->line);
				} else {
					emit_byte(emitter, stmt->default_value, statement->line);
				}
			}

			if (emitter->function->depth == 0) {
				int str = make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(MM(emitter->compiler), stmt->name, strlen(stmt->name))));
				emit_bytes(emitter, OP_DEFINE_GLOBAL, (uint8_t) str, statement->line);
			} else {
				emit_bytes(emitter, OP_SET_LOCAL, (uint8_t) add_local(emitter, stmt->name), statement->line);
			}

			break;
		}
		case EXPRESSION_STATEMENT:
			emit_expression(emitter, ((LitExpressionStatement*) statement)->expr);
			emit_byte(emitter, OP_POP, statement->line);

			break;
		case IF_STATEMENT: {
			LitIfStatement* stmt = (LitIfStatement*) statement;
			emit_expression(emitter, stmt->condition);

			uint64_t else_jump = emit_jump(emitter, OP_JUMP_IF_FALSE, statement->line);
			emit_statement(emitter, stmt->if_branch);

			uint64_t end_jump = emit_jump(emitter, OP_JUMP, statement->line);
			uint64_t end_jumps[stmt->else_if_branches == NULL ? 0 : stmt->else_if_branches->count];

			if (stmt->else_if_branches != NULL) {
				for (int i = 0; i < stmt->else_if_branches->count; i++) {
					patch_jump(emitter, else_jump);
					emit_expression(emitter, stmt->else_if_conditions->values[i]);
					else_jump = emit_jump(emitter, OP_JUMP_IF_FALSE, statement->line);
					emit_statement(emitter, stmt->else_if_branches->values[i]);

					end_jumps[i] = emit_jump(emitter, OP_JUMP, statement->line);
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
			}

			break;
		}
		case WHILE_STATEMENT: {
			LitWhileStatement* stmt = (LitWhileStatement*) statement;

			uint64_t loop_start = emitter->function->function->chunk.count;
			emitter->loop_start = loop_start; // Save for continue statements

			emit_expression(emitter, stmt->condition);
			uint64_t exit_jump = emit_jump(emitter, OP_JUMP_IF_FALSE, statement->line);
			emit_byte(emitter, OP_POP, statement->line);

			emit_statement(emitter, stmt->body);
			emit_loop(emitter, loop_start, statement->line);
			patch_jump(emitter, exit_jump);
			emit_byte(emitter, OP_POP, statement->line);

			// Patch breaks
			for (int i = 0; i < emitter->breaks.count; i++) {
				patch_jump(emitter, emitter->breaks.values[i]);
			}

			// Clear the array, it automaticly calls lit_init_ints() too
			lit_free_ints(MM(emitter->compiler), &emitter->breaks);
			break;
		}
		case FUNCTION_STATEMENT: {
			LitFunctionStatement* stmt = (LitFunctionStatement*) statement;
			LitEmitterFunction function;

			function.function = lit_new_function(MM(emitter->compiler));
			function.depth = emitter->function->depth + 1;
			function.local_count = 0;
			function.enclosing = emitter->function;
			function.function = lit_new_function(MM(emitter->compiler));
			function.function->name = lit_copy_string(MM(emitter->compiler), stmt->name, (int) strlen(stmt->name));
			function.function->arity = stmt->parameters == NULL ? 0 : stmt->parameters->count;

			emitter->function = &function;

			if (stmt->parameters != NULL) {
				for (int i = 0; i < stmt->parameters->count; i++) {
					add_local(emitter, stmt->parameters->values[i].name);
				}
			}

			emit_statement(emitter, stmt->body);

			if (DEBUG_TRACE_CODE) {
				lit_trace_chunk(MM(emitter->compiler), &function.function->chunk, stmt->name);
			}

			emitter->function = function.enclosing;
			emit_bytes(emitter, OP_CLOSURE, make_constant(emitter, MAKE_OBJECT_VALUE(function.function)), statement->line);

			for (int i = 0; i < function.function->upvalue_count; i++) {
				emit_byte(emitter, (uint8_t) (function.upvalues[i].local ? 1 : 0), statement->line);
				emit_byte(emitter, function.upvalues[i].index, statement->line);
			}

			if (emitter->function->depth == 0) {
				emit_bytes(emitter, OP_DEFINE_GLOBAL, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(MM(emitter->compiler), stmt->name, strlen(stmt->name)))), statement->line);
			} else {
				emit_bytes(emitter, OP_SET_LOCAL, (uint8_t) add_local(emitter, stmt->name), statement->line);
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

			emit_byte(emitter, OP_RETURN, statement->line);
			break;
		}
		case CLASS_STATEMENT: {
			LitClassStatement* stmt = (LitClassStatement*) statement;

			if (stmt->super != NULL) {
				emit_expression(emitter, (LitExpression*) stmt->super);
				emit_bytes(emitter, OP_SUBCLASS, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(MM(emitter->compiler), stmt->name, strlen(stmt->name)))), statement->line);
			} else {
				emit_bytes(emitter, OP_CLASS, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(MM(emitter->compiler), stmt->name, strlen(stmt->name)))), statement->line);
			}

			if (stmt->fields != NULL) {
				for (int i = 0; i < stmt->fields->count; i++) {
					LitFieldStatement* field = (LitFieldStatement*) stmt->fields->values[i];

					if (field->init != NULL) {
						emit_expression(emitter, field->init);
					} else {
						if (strcmp(field->type, "bool") == 0) {
							emit_byte(emitter, OP_FALSE, statement->line);
						} else if (strcmp(field->type, "int") == 0 || strcmp(field->type, "double") == 0) {
							emit_constant(emitter, MAKE_NUMBER_VALUE(0), statement->line);
						} else if (strcmp(field->type, "char") == 0) {
							emit_constant(emitter, MAKE_CHAR_VALUE('\0'), statement->line);
						} else {
							emit_byte(emitter, OP_NIL, statement->line);
						}
					}

					emit_bytes(emitter, field->is_static ? OP_DEFINE_STATIC_FIELD : OP_DEFINE_FIELD, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(MM(emitter->compiler), field->name, strlen(field->name)))), statement->line);
				}
			}

			if (stmt->methods != NULL) {
				for (int j = 0; j < stmt->methods->count; j++) {
					LitMethodStatement* method = stmt->methods->values[j];
					LitEmitterFunction function;

					function.function = lit_new_function(MM(emitter->compiler));
					function.depth = emitter->function->depth + 1;
					function.local_count = 1;
					function.enclosing = emitter->function;
					function.function = lit_new_function(MM(emitter->compiler));

					size_t name_len = strlen(method->name);
					size_t type_len = strlen(stmt->name);

					char* name = (char*) reallocate(MM(emitter->compiler), NULL, 0, name_len + type_len + 1);

					strncpy(name, stmt->name, type_len);
					name[type_len] = '.';
					strncpy(&name[type_len + 1], method->name, name_len);

					function.function->name = lit_copy_string(MM(emitter->compiler), name, name_len + type_len + 1);
					function.function->arity = method->parameters == NULL ? 0 : method->parameters->count;

					emitter->function = &function;
					add_local(emitter, "this");

					if (method->parameters != NULL) {
						for (int i = 0; i < method->parameters->count; i++) {
							add_local(emitter, method->parameters->values[i].name);
						}
					}

					if (method->body != NULL) {
						emit_statement(emitter, method->body);
					}

					if (DEBUG_TRACE_CODE) {
						lit_trace_chunk(MM(emitter->compiler), &function.function->chunk, method->name);
					}

					emitter->function = function.enclosing;
					emit_bytes(emitter, OP_CLOSURE, make_constant(emitter, MAKE_OBJECT_VALUE(function.function)), statement->line);

					for (int i = 0; i < function.function->upvalue_count; i++) {
						emit_byte(emitter, (uint8_t) (function.upvalues[i].local ? 1 : 0), statement->line);
						emit_byte(emitter, function.upvalues[i].index, statement->line);
					}

					emit_bytes(emitter, method->is_static ? OP_DEFINE_STATIC_METHOD : OP_DEFINE_METHOD, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(MM(emitter->compiler), method->name, strlen(method->name)))), statement->line);
				}
			}

			emit_bytes(emitter, OP_DEFINE_GLOBAL, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(MM(emitter->compiler), stmt->name, strlen(stmt->name)))), statement->line);
			break;
		}
		case METHOD_STATEMENT: {
			printf("Field or method statement never should be emitted with emit_statement\n");
			UNREACHABLE();
		}
		case BREAK_STATEMENT: {
			lit_ints_write(MM(emitter->compiler), &emitter->breaks, emit_jump(emitter, OP_JUMP, statement->line));
			break;
		}
		case CONTINUE_STATEMENT: {
			emit_jump_instant(emitter, OP_JUMP, emitter->loop_start, statement->line);
			break;
		}
		default: {
			printf("Unknown statement with id %i!\n", statement->type);
			UNREACHABLE();
		}
	}
}

static void emit_statements(LitEmitter* emitter, LitStatements* statements) {
	for (int i = 0; i < statements->count; i++) {
		emit_statement(emitter, statements->values[i]);
	}
}

void lit_init_emitter(LitCompiler* compiler, LitEmitter* emitter) {
	emitter->compiler = compiler;
	emitter->function = NULL;
	emitter->class = NULL;

	lit_init_ints(&emitter->breaks);
}

void lit_free_emitter(LitEmitter* emitter) {
	lit_free_ints(MM(emitter->compiler), &emitter->breaks);
}

LitFunction* lit_emit(LitEmitter* emitter, LitStatements* statements) {
	emitter->had_error = false;

	LitEmitterFunction function;
	LitFunction* fn = lit_new_function(MM(emitter->compiler));

	fn->name = lit_copy_string(MM(emitter->compiler), "$main", 5);

	function.function = fn;
	function.depth = 0;
	function.local_count = 0;
	function.enclosing = NULL;

	emitter->function = &function;

	emit_statements(emitter, statements);
	emit_byte(emitter, OP_NIL, 0);
	emit_byte(emitter, OP_RETURN, 0);

	return emitter->had_error ? NULL : function.function;
}