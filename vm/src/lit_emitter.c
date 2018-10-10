#include <stdio.h>

#include "lit_emitter.h"

static void emit_byte(LitEmitter* emitter, uint8_t byte) {
	lit_chunk_write(emitter->vm, &emitter->function->chunk, byte);
}

static void emit_bytes(LitEmitter* emitter, uint8_t a, uint8_t b) {
	lit_chunk_write(emitter->vm, &emitter->function->chunk, a);
	lit_chunk_write(emitter->vm, &emitter->function->chunk, b);
}

static uint8_t make_constant(LitEmitter* emitter, LitValue value) {
	int constant = lit_chunk_add_constant(emitter->vm, &emitter->function->chunk, value);

	if (constant > UINT8_MAX) {
		printf("Too many constants in one chunk\n");
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

	return emitter->function->chunk.count - 2;
}

static void patch_jump(LitEmitter* emitter, int offset) {
	LitChunk* chunk = &emitter->function->chunk;
	int jump = chunk->count - offset - 2;

	if (jump > UINT16_MAX) {
		printf("Too much code to jump over\n");
	}

	chunk->code[offset] = (uint8_t) ((jump >> 8) & 0xff);
	chunk->code[offset + 1] = (uint8_t) (jump & 0xff);
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
		case VAR_EXPRESSION: break;
		case ASSIGN_EXPRESSION: break;
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
		case CALL_EXPRESSION: break;
	}
}

static void emit_expressions(LitEmitter* emitter, LitExpressions* expressions) {
	for (int i = 0; i < expressions->count; i++) {
		emit_expression(emitter, expressions->values[i]);
	}
}

static void emit_statement(LitEmitter* emitter, LitStatement* statement) {
	switch (statement->type) {
		case VAR_STATEMENT: break;
		case EXPRESSION_STATEMENT:
			emit_expression(emitter, ((LitExpressionStatement*) statement)->expr);
			emit_byte(emitter, OP_POP);
			break;
		case IF_STATEMENT: break;
		case BLOCK_STATEMENT: break;
		case WHILE_STATEMENT: break;
		case FUNCTION_STATEMENT: break;
		case RETURN_STATEMENT: break;
	}
}

static void emit_statements(LitEmitter* emitter, LitStatements* statements) {
	for (int i = 0; i < statements->count; i++) {
		emit_statement(emitter, statements->values[i]);
	}
}

static void init_emitter(LitEmitter* emitter) {
	emitter->function = lit_new_function(emitter->vm);
	emitter->depth = 0;
}

static void free_emitter(LitEmitter* emitter) {

}

LitFunction* lit_emit(LitVm* vm, LitStatements* statements) {
	LitEmitter emitter;
	emitter.vm = vm;

	init_emitter(&emitter);
	emit_statements(&emitter, statements);
	emit_byte(&emitter, OP_NIL);
	emit_byte(&emitter, OP_RETURN);

	LitFunction *function = emitter.function;
	free_emitter(&emitter);

	return function;
}