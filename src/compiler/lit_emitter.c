#include <stdio.h>
#include <memory.h>
#include <zconf.h>
#include <lit.h>

#include <lit_debug.h>

#include <compiler/lit_emitter.h>
#include <vm/lit_memory.h>
#include <compiler/lit_ast.h>

DEFINE_ARRAY(LitInts, uint64_t, ints)
DEFINE_ARRAY(LitShorts, uint16_t, shorts)

static void emit_byte(LitEmitter* emitter, uint8_t byte, uint64_t line) {
	lit_chunk_write(MM(emitter->compiler), &emitter->function->function->chunk, byte, line);
}

static void emit_byte2(LitEmitter* emitter, uint8_t a, uint8_t b, uint64_t line) {
	lit_chunk_write(MM(emitter->compiler), &emitter->function->function->chunk, a, line);
	lit_chunk_write(MM(emitter->compiler), &emitter->function->function->chunk, b, line);
}

static void emit_byte3(LitEmitter* emitter, uint8_t a, uint8_t b, uint8_t c, uint64_t line) {
	lit_chunk_write(MM(emitter->compiler), &emitter->function->function->chunk, a, line);
	lit_chunk_write(MM(emitter->compiler), &emitter->function->function->chunk, b, line);
	lit_chunk_write(MM(emitter->compiler), &emitter->function->function->chunk, c, line);
}

static void emit_byte4(LitEmitter* emitter, uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint64_t line) {
	lit_chunk_write(MM(emitter->compiler), &emitter->function->function->chunk, a, line);
	lit_chunk_write(MM(emitter->compiler), &emitter->function->function->chunk, b, line);
	lit_chunk_write(MM(emitter->compiler), &emitter->function->function->chunk, c, line);
	lit_chunk_write(MM(emitter->compiler), &emitter->function->function->chunk, d, line);
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

static uint16_t reserve_register(LitEmitter* emitter) {
	if (emitter->function->free_registers.count == 0) {
		return emitter->function->register_count++;
	}

	emitter->function->free_registers.count--;
	return emitter->function->free_registers.values[emitter->function->free_registers.count];
}

static void free_register(LitEmitter* emitter, uint16_t reg) {
	lit_shorts_write(MM(emitter->compiler), &emitter->function->free_registers, reg);
}

static uint8_t make_constant(LitEmitter* emitter, LitValue value) {
	int constant = lit_chunk_add_constant(MM(emitter->compiler), &emitter->function->function->chunk, value);

	if (constant > UINT16_MAX) {
		error(emitter, "Too many constants in one chunk");
		return 0;
	}

	return (uint8_t) constant;
}

static uint16_t emit_constant(LitEmitter* emitter, LitValue constant, uint64_t line) {
	int id = lit_chunk_add_constant(MM(emitter->compiler), &emitter->function->function->chunk, constant);

	if (id > UINT16_MAX) {
		error(emitter, "Too many constants in one chunk");
		return 0;
	}

	uint16_t reg = reserve_register(emitter);

	if (id > UINT8_MAX) {
		// Id wont fit into a single byte, put it into 2 bytes
		emit_byte4(emitter, OP_CONSTANT_LONG, (uint8_t) reg, (uint8_t) ((id >> 8) & 0xff), (uint8_t) (id & 0xff), line);
	} else {
		emit_byte3(emitter, OP_CONSTANT, (uint8_t) reg, (uint8_t) id, line);
	}

	return (uint16_t) (reg);
}

static void emit_statement(LitEmitter* emitter, LitStatement* statement);

static uint16_t emit_expression(LitEmitter* emitter, LitExpression* expression) {
	switch (expression->type) {
		case BINARY_EXPRESSION: {
			LitBinaryExpression* expr = (LitBinaryExpression*) expression;

			uint16_t a = emit_expression(emitter, expr->left);
			uint16_t b = emit_expression(emitter, expr->right);
			uint16_t c = reserve_register(emitter);

			switch (expr->operator) {
				// FIXME: support OP_ADD_LONG, etc
				case TOKEN_PLUS: emit_byte4(emitter, OP_ADD, (uint8_t) c, (uint8_t) a, (uint8_t) b, expression->line); break;
				case TOKEN_MINUS: emit_byte4(emitter, OP_SUBTRACT, (uint8_t) c, (uint8_t) a, (uint8_t) b, expression->line); break;
				case TOKEN_STAR: emit_byte4(emitter, OP_MULTIPLY, (uint8_t) c, (uint8_t) a, (uint8_t) b, expression->line); break;
				case TOKEN_SLASH: emit_byte4(emitter, OP_DIVIDE, (uint8_t) c, (uint8_t) a, (uint8_t) b, expression->line); break;
				case TOKEN_PERCENT: emit_byte4(emitter, OP_MODULO, (uint8_t) c, (uint8_t) a, (uint8_t) b, expression->line); break;
				case TOKEN_CARET: emit_byte4(emitter, OP_POWER, (uint8_t) c, (uint8_t) a, (uint8_t) b, expression->line); break;
				case TOKEN_CELL: emit_byte4(emitter, OP_ROOT, (uint8_t) c, (uint8_t) a, (uint8_t) b, expression->line); break;
				default: UNREACHABLE();
			}

			if (expr->left->type == LITERAL_EXPRESSION) {
				free_register(emitter, a);
			}

			if (expr->right->type == LITERAL_EXPRESSION) {
				free_register(emitter, b);
			}

			return c;
		}
		case UNARY_EXPRESSION: {
			LitUnaryExpression* expr = (LitUnaryExpression*) expression;

			uint16_t a = emit_expression(emitter, expr->right);
			uint16_t b = reserve_register(emitter);

			switch (expr->operator) {
				case TOKEN_BANG: emit_byte3(emitter, OP_NOT, (uint8_t) b, (uint8_t) a, expression->line); break;
				default: UNREACHABLE();
			}

			if (expr->right->type == LITERAL_EXPRESSION) {
				free_register(emitter, a);
			}

			return b;
		}
		case LITERAL_EXPRESSION: {
			return emit_constant(emitter, ((LitLiteralExpression*) expression)->value, expression->line);
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

static void emit_statement(LitEmitter* emitter, LitStatement* statement) {
	switch (statement->type) {
		case EXPRESSION_STATEMENT: {
			emit_expression(emitter, ((LitExpressionStatement*) statement)->expr);
			break;
		}
		case FUNCTION_STATEMENT: {
			LitFunctionStatement* stmt = (LitFunctionStatement*) statement;
			LitEmitterFunction function;

			function.function = lit_new_function(MM(emitter->compiler));
			function.register_count = 0;
			function.depth = emitter->function->depth + 1;
			function.local_count = 0;
			function.enclosing = emitter->function;
			function.function = lit_new_function(MM(emitter->compiler));
			function.function->name = lit_copy_string(MM(emitter->compiler), stmt->name, (int) strlen(stmt->name));
			function.function->arity = stmt->parameters == NULL ? 0 : stmt->parameters->count;
			lit_init_shorts(&function.free_registers);

			emitter->function = &function;

			/*if (stmt->parameters != NULL) {
				for (int i = 0; i < stmt->parameters->count; i++) {
					add_local(emitter, stmt->parameters->values[i].name);
				}
			}*/

			emit_statement(emitter, stmt->body);

			if (DEBUG_TRACE_CODE) {
				lit_trace_chunk(MM(emitter->compiler), &function.function->chunk, stmt->name);
			}

			emitter->function = function.enclosing;
			uint16_t id = make_constant(emitter, MAKE_OBJECT_VALUE(function.function));
			uint16_t reg = reserve_register(emitter);

			if (id > UINT8_MAX) {
				// Id wont fit into a single byte, put it into 2 bytes
				emit_byte4(emitter, OP_DEFINE_FUNCTION_LONG, (uint8_t) reg, (uint8_t) ((id >> 8) & 0xff), (uint8_t) (id & 0xff), statement->line);
			} else {
				emit_byte3(emitter, OP_DEFINE_FUNCTION, (uint8_t) reg, (uint8_t) id, statement->line);
			}

			/*
			for (int i = 0; i < function.function->upvalue_count; i++) {
				emit_byte(emitter, (uint8_t) (function.upvalues[i].local ? 1 : 0), statement->line);
				emit_byte(emitter, function.upvalues[i].index, statement->line);
			}*/

			/*
			if (emitter->function->depth == 0) {
				emit_bytes(emitter, OP_DEFINE_GLOBAL, make_constant(emitter, MAKE_OBJECT_VALUE(lit_copy_string(MM(emitter->compiler), stmt->name, strlen(stmt->name)))), statement->line);
			} else {
				emit_bytes(emitter, OP_SET_LOCAL, (uint8_t) add_local(emitter, stmt->name), statement->line);
			}*/

			emitter->function = function.enclosing;
			lit_free_shorts(MM(emitter->compiler), &function.free_registers);

			break;
		}
		case BLOCK_STATEMENT: {
			LitBlockStatement* stmt = ((LitBlockStatement*) statement);

			if (stmt->statements != NULL) {
				emit_statements(emitter, stmt->statements);
			}

			break;
		}
		case RETURN_STATEMENT: {
			LitReturnStatement* stmt = (LitReturnStatement*) statement;

			if (stmt->value == NULL) {
				emit_byte(emitter, OP_EXIT, statement->line);
			} else {
				uint16_t reg = emit_expression(emitter, stmt->value);
				emit_byte2(emitter, OP_RETURN, (uint8_t) reg, statement->line);
			}

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

	function.register_count = 0;
	lit_init_shorts(&function.free_registers);

	LitFunction* fn = lit_new_function(MM(emitter->compiler));
	fn->name = lit_copy_string(MM(emitter->compiler), "$main", 5);

	function.function = fn;
	function.enclosing = NULL;

	emitter->function = &function;

	emit_statements(emitter, statements);
	emit_byte(emitter, OP_EXIT, 0);

	lit_free_shorts(MM(emitter->compiler), &function.free_registers);
	return emitter->had_error ? NULL : function.function;
}