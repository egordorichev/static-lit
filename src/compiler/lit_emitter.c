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
	if (emitter->free_registers.count == 0) {
		return emitter->register_count++;
	}

	emitter->free_registers.count--;
	return emitter->free_registers.values[emitter->free_registers.count];
}

static void free_register(LitEmitter* emitter, uint16_t reg) {
	lit_shorts_write(MM(emitter->compiler), &emitter->free_registers, reg);
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
		emit_byte4(emitter, OP_CONSTANT, (uint8_t) reg, (uint8_t) ((id >> 8) & 0xff), (uint8_t) (id & 0xff), line);
	} else {
		emit_byte3(emitter, OP_CONSTANT, (uint8_t) reg, (uint8_t) id, line);
	}

	return (uint16_t) reg;
}

static void emit_statement(LitEmitter* emitter, LitStatement* statement);

static uint16_t emit_expression(LitEmitter* emitter, LitExpression* expression) {
	switch (expression->type) {
		case BINARY_EXPRESSION: {
			LitBinaryExpression* expr = (LitBinaryExpression*) expression;

			uint16_t a = emit_expression(emitter, expr->left);
			uint16_t b = emit_expression(emitter, expr->right);

			switch (expr->operator) {
				// FIXME: support OP_ADD_LONG, etc
				case TOKEN_PLUS: emit_byte4(emitter, OP_ADD, (uint8_t) a, (uint8_t) a, (uint8_t) b, expression->line); break;
				case TOKEN_MINUS: emit_byte4(emitter, OP_SUBTRACT, (uint8_t) a, (uint8_t) a, (uint8_t) b, expression->line); break;
				case TOKEN_STAR: emit_byte4(emitter, OP_MULTIPLY, (uint8_t) a, (uint8_t) a, (uint8_t) b, expression->line); break;
				case TOKEN_SLASH: emit_byte4(emitter, OP_DIVIDE, (uint8_t) a, (uint8_t) a, (uint8_t) b, expression->line); break;
				case TOKEN_PERCENT: emit_byte4(emitter, OP_MODULO, (uint8_t) a, (uint8_t) a, (uint8_t) b, expression->line); break;
				case TOKEN_CARET: emit_byte4(emitter, OP_POWER, (uint8_t) a, (uint8_t) a, (uint8_t) b, expression->line); break;
				case TOKEN_CELL: emit_byte4(emitter, OP_ROOT, (uint8_t) a, (uint8_t) a, (uint8_t) b, expression->line); break;
				default: UNREACHABLE();
			}

			free_register(emitter, b);
			return a;
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

	lit_init_shorts(&emitter->free_registers);
	lit_init_ints(&emitter->breaks);
}

void lit_free_emitter(LitEmitter* emitter) {
	lit_free_ints(MM(emitter->compiler), &emitter->breaks);
	lit_free_shorts(MM(emitter->compiler), &emitter->free_registers);
}

LitFunction* lit_emit(LitEmitter* emitter, LitStatements* statements) {
	emitter->had_error = false;

	LitEmitterFunction function;
	LitFunction* fn = lit_new_function(MM(emitter->compiler));

	fn->name = lit_copy_string(MM(emitter->compiler), "$main", 5);

	function.function = fn;
	function.enclosing = NULL;

	emitter->function = &function;

	emit_statements(emitter, statements);
	emit_byte(emitter, OP_EXIT, 0);

	return emitter->had_error ? NULL : function.function;
}