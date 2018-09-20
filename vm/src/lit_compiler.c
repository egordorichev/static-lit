#include <stdio.h>
#include <stdlib.h>
#include <lit_object.h>
#include <string.h>

#include "lit_compiler.h"
#include "lit_vm.h"
#include "lit_debug.h"

void lit_init_compiler(LitVm* vm, LitCompiler* compiler, LitCompiler* enclosing, LitFunctionType type) {
	compiler->vm = vm;
	compiler->depth = 0;
	compiler->depth = 0;
	compiler->type = type;
	compiler->enclosing = enclosing;
	compiler->function = lit_new_function(vm);
	compiler->local_count = 0;

	switch (type) {
		default: compiler->function->name = NULL;
	}
}

void lit_free_compiler(LitCompiler* compiler) {
	lit_free_lexer(&compiler->lexer);
}

/*
 * Error handling
 */

static void error_at(LitCompiler* compiler, LitToken* token, const char* message) {
	compiler->lexer.panic_mode = true;

	if (compiler->lexer.panic_mode) {
		// Skip errors till we get to normal
		return;
	}

	fprintf(stderr, "[line %d] Error", token->line);

	if (token->type == TOKEN_EOF) {
		fprintf(stderr, " at end");
	} else if (token->type != TOKEN_ERROR) {
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);
	compiler->lexer.had_error = true;
}

static inline void error(LitCompiler* compiler, const char* message) {
	error_at(compiler, &compiler->lexer.previous, message);
}

static inline void error_at_compiler(LitCompiler* compiler, const char* message) {
	error_at(compiler, &compiler->lexer.current, message);
}

/*
 * Utils for parsing tokens
 */

static void advance(LitCompiler* compiler) {
	compiler->lexer.previous = compiler->lexer.current;

	for (;;) {
		compiler->lexer.current = lit_lexer_next_token(&compiler->lexer);

		// printf("Token: %i\n", compiler->lexer.compiler.type);

		if (compiler->lexer.current.type != TOKEN_ERROR) {
			break;
		}

		error_at_compiler(&compiler, compiler->lexer.current.start);
	}
}

static void consume(LitCompiler* compiler, LitTokenType type, const char* message) {
	if (compiler->lexer.current.type == type) {
		advance(compiler);
		return;
	}

	error_at_compiler(compiler, message);
}

static inline bool check(LitCompiler* compiler, LitTokenType type) {
	return compiler->lexer.current.type == type;
}

static bool match(LitCompiler* compiler, LitTokenType type) {
	if (!check(compiler, type)) {
		return false;
	}

	advance(compiler);
	return true;
}

/*
 * Bytecode utils
 */

static inline void emit_byte(LitCompiler* compiler, uint8_t byte) {
	lit_chunk_write(compiler->vm, &compiler->function->chunk, byte, compiler->lexer.previous.line);
}

static void emit_bytes(LitCompiler* compiler, uint8_t a, uint8_t b) {
	lit_chunk_write(compiler->vm, &compiler->function->chunk, a, compiler->lexer.previous.line);
	lit_chunk_write(compiler->vm, &compiler->function->chunk, b, compiler->lexer.previous.line);
}

static uint8_t make_constant(LitCompiler* compiler, LitValue value) {
	int constant = lit_chunk_add_constant(compiler->vm, &compiler->function->chunk, value);

	if (constant > UINT8_MAX) {
		error(compiler, "Too many constants in one chunk");
		return 0;
	}

	return (uint8_t) constant;
}

static inline void emit_constant(LitCompiler* compiler, LitValue value) {
	emit_bytes(compiler, OP_CONSTANT, make_constant(compiler, value));
}

static int emit_jump(LitCompiler* compiler, uint8_t instruction) {
	emit_byte(compiler, instruction);
	emit_bytes(compiler, 0xff, 0xff);

	return compiler->function->chunk.count - 2;
}

static void patch_jump(LitCompiler* compiler, int offset) {
	LitChunk chunk = compiler->function->chunk;
	int jump = chunk.count - offset - 2;

	if (jump > UINT16_MAX) {
		error(compiler, "Too much code to jump over");
	}

	chunk.code[offset] = (jump >> 8) & 0xff;
	chunk.code[offset + 1] = jump & 0xff;
}

static void emit_loop(LitCompiler* compiler, int loopStart) {
	emit_byte(compiler, OP_LOOP);

	int offset = compiler->function->chunk.count - loopStart + 2;

	if (offset > UINT16_MAX) {
		error(compiler, "Loop body is too large");
	}

	emit_bytes(compiler, (offset >> 8) & 0xff, offset & 0xff);
}

/*
 * Actuall compilation
 */

/*
 * Variables
 */

static uint8_t make_identifier_constant(LitCompiler* compiler, LitToken* name) {
	return make_constant(compiler, MAKE_OBJECT_VALUE(lit_copy_string(compiler->vm, name->start, name->length)));
}

static bool identifiers_equal(LitToken* a, LitToken* b) {
	if (a->length != b->length) {
		return false;
	}

	return memcmp(a->start, b->start, a->length) == 0;
}

static void begin_scope(LitCompiler* compiler) {
	compiler->depth++;
}

static void end_scope(LitCompiler* compiler) {
	compiler->depth--;

	while (compiler->local_count > 0 &&
		compiler->locals[compiler->local_count - 1].depth > compiler->depth) {

		if (compiler->locals[compiler->local_count - 1].upvalue) {
			emit_byte(compiler, OP_CLOSE_UPVALUE);
		} else {
			emit_byte(compiler, OP_POP);
		}

		compiler->local_count--;
	}
}

static int resolve_local(LitCompiler* compiler, LitToken* name, bool inFunction) {
	for (int i = compiler->local_count - 1; i >= 0; i--) {
		LitLocal* local = &compiler->locals[i];

		if (identifiers_equal(name, &local->name)) {
			if (!inFunction && local->depth == -1) {
				error(compiler, "Cannot read local variable in its own initializer");
			}
			return i;
		}
	}

	return -1;
}

static int add_upvalue(LitCompiler* compiler, uint8_t index, bool is_local) {
	int upvalueCount = compiler->function->upvalue_count;

	for (int i = 0; i < upvalueCount; i++) {
		LitCompilerUpvalue* upvalue = &compiler->upvalues[i];

		if (upvalue->index == index && upvalue->local == is_local) {
			return i;
		}
	}

	if (upvalueCount == UINT8_COUNT) {
		error(compiler, "Too many closure variables in function");
		return 0;
	}

	compiler->upvalues[upvalueCount].local = is_local;
	compiler->upvalues[upvalueCount].index = index;

	return compiler->function->upvalue_count++;
}

static int resolve_upvalue(LitCompiler* compiler, LitToken* name) {
	if (compiler->enclosing == NULL) {
		return -1;
	}

	int local = resolve_local(compiler->enclosing, name, true);

	if (local != -1) {
		compiler->enclosing->locals[local].upvalue = true;
		return add_upvalue(compiler, (uint8_t)local, true);
	}

	int upvalue = resolve_upvalue(compiler->enclosing, name);

	if (upvalue != -1) {
		return add_upvalue(compiler, (uint8_t)upvalue, false);
	}

	return -1;
}

static void add_local(LitCompiler* compiler, LitToken name) {
	if (compiler->local_count == UINT8_COUNT) {
		error(compiler, "Too many local variables in function");
		return;
	}

	LitLocal* local = &compiler->locals[compiler->local_count];

	local->name = name;
	local->depth = -1;
	local->upvalue = false;
	compiler->local_count++;
}

static void declare_variable(LitCompiler* compiler) {
	if (compiler->depth == 0) {
		return;
	}

	LitToken* name = &compiler->lexer.previous;

	for (int i = compiler->local_count - 1; i >= 0; i--) {
		LitLocal* local = &compiler->locals[i];

		if (local->depth != -1 && local->depth < compiler->depth) {
			break;
		}

		if (identifiers_equal(name, &local->name)) {
			error(compiler, "Variable with this name already declared in this scope.");
		}
	}

	add_local(compiler, *name);
}

static uint8_t parse_variable(LitCompiler* compiler, const char* errorMessage) {
	consume(compiler, TOKEN_IDENTIFIER, errorMessage);

	if (compiler->depth == 0) {
		return make_identifier_constant(compiler, &compiler->lexer.previous);
	}

	declare_variable(compiler);
	return 0;
}

static void define_variable(LitCompiler* compiler, uint8_t global) {
	if (compiler->depth == 0) {
		emit_bytes(compiler, OP_DEFINE_GLOBAL, global);
	} else {
		compiler->locals[compiler->local_count - 1].depth = compiler->depth;
	}
}


/*
 * Parsing
 */

static void parse_expression(LitCompiler* compiler);
static void parse_precedence(LitCompiler* compiler, LitPrecedence precedence);
static void parse_declaration(LitCompiler* compiler);
static void parse_statement(LitCompiler* compiler);

static void init_parse_rules();
static LitParseRule parse_rules[TOKEN_EOF + 1];
static bool inited_parse_rules = false;

static inline LitParseRule* get_parse_rule(LitTokenType type) {
	return &parse_rules[type];
}

static void named_variable(LitCompiler* compiler, LitToken name, bool can_assign) {
	uint8_t getOp, setOp;
	int arg = resolve_local(compiler, &name, false);

	// TODO: can be speeded up via removing vars
	if (arg != -1) {
		getOp = OP_GET_LOCAL;
		setOp = OP_SET_LOCAL;
	} else if ((arg = resolve_upvalue(compiler, &name)) != -1) {
		getOp = OP_GET_UPVALUE;
		setOp = OP_SET_UPVALUE;
	} else {
		arg = make_identifier_constant(compiler, &name);
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}

	if (can_assign && match(compiler, TOKEN_EQUAL)) {
		parse_expression(compiler);
		emit_bytes(compiler, setOp, (uint8_t) arg);
	} else {
		emit_bytes(compiler, getOp, (uint8_t) arg);
	}
}

static void parse_variable_usage(LitCompiler* compiler, bool can_assign) {
	named_variable(compiler, compiler->lexer.previous, can_assign);
}

static void parse_block(LitCompiler* compiler) {
	while (!check(compiler, TOKEN_RIGHT_BRACE) && !check(compiler, TOKEN_EOF)) {
		parse_declaration(compiler);
	}

	consume(compiler, TOKEN_RIGHT_BRACE, "Expected '}' after block");
}

static void parse_number(LitCompiler* compiler, bool can_assign) {
	emit_constant(compiler, MAKE_NUMBER_VALUE(strtod(compiler->lexer.previous.start, NULL)));
}

static void parse_grouping(LitCompiler* compiler, bool can_assign) {
	parse_expression(compiler);
	consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
}

static void parse_unary(LitCompiler* compiler, bool can_assign) {
	LitTokenType operatorType = compiler->lexer.previous.type;
	parse_precedence(compiler, PREC_UNARY);

	switch (operatorType) {
		case TOKEN_BANG: emit_byte(compiler, OP_NOT); break;
		case TOKEN_MINUS: emit_byte(compiler, OP_NEGATE); break;
		default: UNREACHABLE();
	}
}

static void parse_binary(LitCompiler* compiler, bool can_assign) {
	LitTokenType operator_type = compiler->lexer.previous.type;

	LitParseRule* rule = get_parse_rule(operator_type);
	parse_precedence(compiler, (LitPrecedence) (rule->precedence + 1));

	switch (operator_type) {
		case TOKEN_PLUS: emit_byte(compiler, OP_ADD); break;
		case TOKEN_MINUS: emit_byte(compiler, OP_SUBTRACT); break;
		case TOKEN_STAR: emit_byte(compiler, OP_MULTIPLY); break;
		case TOKEN_SLASH: emit_byte(compiler, OP_DIVIDE); break;
		case TOKEN_EQUAL_EQUAL: emit_byte(compiler, OP_EQUAL); break;
		case TOKEN_BANG_EQUAL: emit_byte(compiler, OP_NOT_EQUAL); break;
		case TOKEN_LESS_EQUAL: emit_byte(compiler, OP_LESS_EQUAL); break;
		case TOKEN_GREATER_EQUAL: emit_byte(compiler, OP_GREATER_EQUAL); break;
		case TOKEN_LESS: emit_byte(compiler, OP_LESS); break;
		case TOKEN_GREATER: emit_byte(compiler, OP_GREATER); break;
		default: UNREACHABLE();
	}
}

static void parse_literal(LitCompiler* compiler, bool can_assign) {
	switch (compiler->lexer.previous.type) {
		case TOKEN_NIL: emit_byte(compiler, OP_NIL); break;
		case TOKEN_TRUE: emit_byte(compiler, OP_TRUE); break;
		case TOKEN_FALSE: emit_byte(compiler, OP_FALSE); break;
		default: UNREACHABLE();
	}
}

static void parse_string(LitCompiler* compiler, bool can_assign) {
	emit_constant(compiler, MAKE_OBJECT_VALUE(lit_copy_string(compiler->vm, compiler->lexer.previous.start + 1, compiler->lexer.previous.length - 2)));
}

static uint8_t parse_var_declaration(LitCompiler* compiler) {
	uint8_t global = parse_variable(compiler, "Expected variable name");

	if (match(compiler, TOKEN_EQUAL)) {
		parse_expression(compiler);

		if (compiler->depth > 0) {
			emit_bytes(compiler, OP_SET_LOCAL, global);
		}
	} else {
		emit_byte(compiler, OP_NIL);
	}

	define_variable(compiler, global);
	return global;
}

static void parse_if(LitCompiler* compiler) {
	consume(compiler, TOKEN_LEFT_PAREN, "Expected '(' after 'if'");
	parse_expression(compiler);
	consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')' after condition");

	int else_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);

	emit_byte(compiler, OP_POP);
	parse_statement(compiler);

	int end_jump = emit_jump(compiler, OP_JUMP);

	patch_jump(compiler, else_jump);
	emit_byte(compiler, OP_POP);

	if (match(compiler, TOKEN_ELSE)) {
		parse_statement(compiler);
	}

	patch_jump(compiler, end_jump);
}

static void parse_while(LitCompiler* compiler) {
	int loop_start = compiler->function->chunk.count;

	consume(compiler, TOKEN_LEFT_PAREN, "Expected '(' after 'while'");
	parse_expression(compiler);
	consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')' after condition");

	int exit_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);

	emit_byte(compiler, OP_POP);
	parse_statement(compiler);

	emit_loop(compiler, loop_start);
	patch_jump(compiler, exit_jump);
	emit_byte(compiler, OP_POP);
}

static void parse_for(LitCompiler* compiler) {
	begin_scope(compiler);

	consume(compiler, TOKEN_LEFT_PAREN, "Expected '(' after 'for'.");
	uint8_t var = parse_var_declaration(compiler);

	int loop_start = compiler->function->chunk.count;
	int exit_jump = -1;

	if (match(compiler, TOKEN_RANGE)) {
		emit_bytes(compiler, OP_GET_LOCAL, var);
		parse_expression(compiler);
		emit_byte(compiler, OP_LESS);

		exit_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
		emit_byte(compiler, OP_POP);
	}

	bool has = match(compiler, TOKEN_COMMA);

	int body_jump = emit_jump(compiler, OP_JUMP);
	int increment_start = compiler->function->chunk.count;

	emit_bytes(compiler, OP_GET_LOCAL, var);

	if (has) {
		parse_expression(compiler);
	} else {
		emit_constant(compiler, MAKE_NUMBER_VALUE(1));
	}

	emit_byte(compiler, OP_ADD);
	emit_bytes(compiler, OP_SET_LOCAL, var);
	emit_byte(compiler, OP_POP);
	consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')' after for loop");

	emit_loop(compiler, loop_start);
	loop_start = increment_start;

	patch_jump(compiler, body_jump);

	parse_statement(compiler);

	emit_loop(compiler, loop_start);

	if (exit_jump != -1) {
		patch_jump(compiler, exit_jump);
		emit_byte(compiler, OP_POP);
	}

	end_scope(compiler);
}

static void parse_precedence(LitCompiler* compiler, LitPrecedence precedence) {
	advance(compiler);
	LitParseFn prefix = get_parse_rule(compiler->lexer.previous.type)->prefix;

	if (prefix == NULL) {
		error(compiler, "Expected expression");
		return;
	}

	bool can_assign = precedence <= PREC_ASSIGNMENT;
	prefix(compiler, can_assign);

	while (precedence <= get_parse_rule(compiler->lexer.current.type)->precedence) {
		advance(compiler);
		get_parse_rule(compiler->lexer.previous.type)->infix(compiler, can_assign);
	}

	if (can_assign && match(compiler, TOKEN_EQUAL)) {
		error(compiler, "Invalid assignment target");
		parse_expression(compiler);
	}
}

static void parse_expression(LitCompiler* compiler) {
	parse_precedence(compiler, PREC_ASSIGNMENT);
}

static void parse_statement(LitCompiler* compiler) {
	LitTokenType token = compiler->lexer.current.type;

	if (token == TOKEN_IF) {
		advance(compiler);
		parse_if(compiler);
	} else if (token == TOKEN_WHILE) {
		advance(compiler);
		parse_while(compiler);
	} else if (token == TOKEN_FOR) {
		advance(compiler);
		parse_for(compiler);
	} else if (token == TOKEN_PRINT) {
		advance(compiler);

		parse_expression(compiler);
		emit_byte(compiler, OP_PRINT);
	}	else if (token == TOKEN_LEFT_BRACE) {
		advance(compiler);

		begin_scope(compiler);
		parse_block(compiler);
		end_scope(compiler);
	} else {
		parse_expression(compiler);
		// emit_byte(compiler, OP_POP);
	}
}

static void parse_declaration(LitCompiler* compiler) {
	LitTokenType token = compiler->lexer.current.type;

	if (token == TOKEN_VAR) {
		advance(compiler);
		parse_var_declaration(compiler);
	} else {
		parse_statement(compiler);
	}
}

static LitFunction* end_compiler(LitCompiler *compiler) {
	emit_byte(compiler, OP_RETURN);

#ifdef DEBUG_PRINT_CODE
	lit_trace_chunk(&compiler->function->chunk, "code");
#endif

	LitFunction* function = compiler->function;
	// compiler = compiler->enclosing;

	return function;
}

LitFunction *lit_compile(LitVm* vm, const char* code) {
	if (!inited_parse_rules) {
		init_parse_rules();
		inited_parse_rules = true;
	}

	LitCompiler compiler;
	lit_init_compiler(vm, &compiler, NULL, TYPE_TOP_LEVEL);

	compiler.lexer.vm = vm;
	lit_init_lexer(&compiler.lexer, code);

	advance(&compiler);

	if (!match(&compiler, TOKEN_EOF)) {
		do {
			parse_declaration(&compiler);
		} while (!match(&compiler, TOKEN_EOF));
	}

	LitFunction* function = end_compiler(&compiler);
	bool error = compiler.lexer.had_error;
	lit_free_compiler(&compiler);

	return error ? NULL : function;
}

static void init_parse_rules() {
	parse_rules[TOKEN_LEFT_PAREN] = (LitParseRule) { parse_grouping, NULL, PREC_CALL };
	parse_rules[TOKEN_MINUS] = (LitParseRule) { parse_unary, parse_binary, PREC_TERM };
	parse_rules[TOKEN_PLUS] = (LitParseRule) { NULL, parse_binary, PREC_TERM };
	parse_rules[TOKEN_SLASH] = (LitParseRule) { NULL, parse_binary, PREC_FACTOR };
	parse_rules[TOKEN_STAR] = (LitParseRule) { NULL, parse_binary, PREC_FACTOR };
	parse_rules[TOKEN_NUMBER] = (LitParseRule) { parse_number, NULL, PREC_NONE };
	parse_rules[TOKEN_BANG] = (LitParseRule) { parse_unary, NULL, PREC_NONE };
	parse_rules[TOKEN_NIL] = (LitParseRule) { parse_literal, NULL, PREC_NONE };
	parse_rules[TOKEN_TRUE] = (LitParseRule) { parse_literal, NULL, PREC_NONE };
	parse_rules[TOKEN_FALSE] = (LitParseRule) { parse_literal, NULL, PREC_NONE };
	parse_rules[TOKEN_EQUAL_EQUAL] = (LitParseRule) { NULL, parse_binary, PREC_EQUALITY };
	parse_rules[TOKEN_GREATER] = (LitParseRule) { NULL, parse_binary, PREC_COMPARISON };
	parse_rules[TOKEN_LESS] = (LitParseRule) { NULL, parse_binary, PREC_COMPARISON };
	parse_rules[TOKEN_GREATER_EQUAL] = (LitParseRule) { NULL, parse_binary, PREC_COMPARISON };
	parse_rules[TOKEN_LESS_EQUAL] = (LitParseRule) { NULL, parse_binary, PREC_COMPARISON };
	parse_rules[TOKEN_BANG_EQUAL] = (LitParseRule) { NULL, parse_binary, PREC_EQUALITY };
	parse_rules[TOKEN_STRING] = (LitParseRule) { parse_string, NULL, PREC_NONE };
	parse_rules[TOKEN_IDENTIFIER] = (LitParseRule) { parse_variable_usage, NULL, PREC_NONE };
}