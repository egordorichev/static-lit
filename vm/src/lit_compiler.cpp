#include "lit_compiler.hpp"
#include "lit_common.hpp"

#ifdef DEBUG_PRINT_CODE
#include "lit_debug.hpp"
#endif

static LitCompiler *compiler;

static LitParseRule rules[] = {
	{ parse_grouping, NULL,    PREC_CALL },       // TOKEN_LEFT_PAREN
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_PAREN
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_LEFT_BRACE [big]
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_BRACE
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_COMMA
	{ NULL,     NULL,     PREC_CALL },       // TOKEN_DOT
	{ parse_unary,    parse_binary,  PREC_TERM },       // TOKEN_MINUS
	{ NULL,     parse_binary,  PREC_TERM },       // TOKEN_PLUS
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_SEMICOLON
	{ NULL,     parse_binary,  PREC_FACTOR },     // TOKEN_SLASH
	{ NULL,     parse_binary,  PREC_FACTOR },     // TOKEN_STAR
	{ parse_unary,    NULL,    PREC_NONE },       // TOKEN_BANG
	{ NULL,     parse_binary,  PREC_EQUALITY },   // TOKEN_BANG_EQUAL
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL
	{ NULL,     parse_binary,  PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL
	{ NULL,     parse_binary,  PREC_COMPARISON }, // TOKEN_GREATER
	{ NULL,     parse_binary,  PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
	{ NULL,     parse_binary,  PREC_COMPARISON }, // TOKEN_LESS
	{ NULL,     parse_binary,  PREC_COMPARISON }, // TOKEN_LESS_EQUAL
	{ NULL, NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
	{ NULL,   NULL,    PREC_NONE },       // TOKEN_STRING
	{ parse_number,   NULL,    PREC_NONE },       // TOKEN_NUMBER
	{ NULL,     NULL,    PREC_AND },        // TOKEN_AND
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE
	{ NULL,  NULL,    PREC_NONE },       // TOKEN_FALSE
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_FUN
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_FOR
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_IF
	{ parse_literal,  NULL,    PREC_NONE },       // TOKEN_NIL
	{ NULL,     NULL,     PREC_OR },         // TOKEN_OR
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_PRINT
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_RETURN
	{ NULL,   NULL,    PREC_NONE },       // TOKEN_SUPER
	{ NULL,    NULL,    PREC_NONE },       // TOKEN_THIS
	{ parse_literal,  NULL,    PREC_NONE },       // TOKEN_TRUE
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_VAR
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_WHILE
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_ERROR
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_EOF
}
;
bool inited_rules = false;

static void init_rules() {
	if (true) {
		return;
	}

	inited_rules = true;

	rules[TOKEN_LEFT_PAREN] = { parse_grouping, nullptr, PREC_CALL };
	rules[TOKEN_MINUS] = { parse_unary, parse_binary, PREC_TERM };
	rules[TOKEN_PLUS] = { nullptr, parse_binary, PREC_TERM };
	rules[TOKEN_SLASH] = { nullptr, parse_binary, PREC_FACTOR };
	rules[TOKEN_STAR] = { nullptr, parse_binary, PREC_FACTOR };
	rules[TOKEN_BANG] = { parse_unary, nullptr, PREC_NONE };
	rules[TOKEN_BANG_EQUAL] = { nullptr, parse_binary, PREC_EQUALITY };
	rules[TOKEN_EQUAL] = { nullptr, parse_binary, PREC_NONE };
	rules[TOKEN_EQUAL_EQUAL] = { nullptr, parse_binary, PREC_EQUALITY };
	rules[TOKEN_GREATER] = { nullptr, parse_binary, PREC_COMPARISON };
	rules[TOKEN_GREATER_EQUAL] = { nullptr, parse_binary, PREC_COMPARISON };
	rules[TOKEN_LESS] = { nullptr, parse_binary, PREC_COMPARISON };
	rules[TOKEN_LESS_EQUAL] = { nullptr, parse_binary, PREC_COMPARISON };
	rules[TOKEN_NIL] = { parse_literal, nullptr, PREC_NONE };
	rules[TOKEN_NUMBER] = { parse_number, nullptr, PREC_NONE };
}

/*
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_RIGHT_PAREN
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_LEFT_BRACE
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_RIGHT_BRACE
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_COMMA
	{ nullptr,     nullptr,    PREC_CALL },       // TOKEN_DOT
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_SEMICOLON
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_IDENTIFIER
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_STRING
	{ nullptr,     nullptr,    PREC_AND },        // TOKEN_AND
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_CLASS
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_ELSE
	{ parse_literal,  nullptr,    PREC_NONE },       // TOKEN_FALSE
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_FUN
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_FOR
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_IF
	{ nullptr,     nullptr,    PREC_OR },         // TOKEN_OR
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_PRINT
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_RETURN
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_SUPER
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_THIS
	{ parse_literal,     nullptr,    PREC_NONE },       // TOKEN_TRUE
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_VAR
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_WHILE
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_ERROR
	{ nullptr,     nullptr,    PREC_NONE },       // TOKEN_EOF
};
*/

void LitCompiler::advance() {
	previous = current;

	for (;;) {
		current = lexer->next_token();

		if (current.type != TOKEN_ERROR) {
			break;
		}

		error_at_current(current.start);
	}
}

void LitCompiler::error_at_current(const char *message) {
	error_at(&current, message);
}

void LitCompiler::error(const char *message) {
	error_at(&previous, message);
}

void LitCompiler::error_at(LitToken *token, const char *message) {
	if (panic_mode) {
		return;
	}

	panic_mode = true;

	fprintf(stderr, "[line %d] Error", token->line);

	if (token->type == TOKEN_EOF) {
		fprintf(stderr, " at end");
	} else if (token->type != TOKEN_ERROR) {
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);
	had_error = true;
}

void LitCompiler::consume(LitTokenType type, const char* message) {
	if (current.type == type) {
		advance();
		return;
	}

	error_at_current(message);
}

void LitCompiler::emit_byte(uint8_t byte) {
	printf("Write %i\n", byte);
	chunk->write(byte, previous.line);
}

void LitCompiler::emit_bytes(uint8_t a, uint8_t b) {
	emit_byte(a);
	emit_byte(b);
}

void LitCompiler::emit_constant(LitValue value) {
	emit_bytes(OP_CONSTANT, make_constant(value));
}

uint8_t LitCompiler::make_constant(LitValue value) {
	int constant = chunk->add_constant(value);

	if (constant > UINT8_MAX) {
		error("Too many constants in one chunk.");
		return 0;
	}

	return (uint8_t) constant;
}

void parse_grouping(bool can_assign) {
	parse_expression();
	compiler->consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

void parse_unary(bool can_assign) {
	LitTokenType type = compiler->get_previous().type;
	parse_precedence(PREC_UNARY);
	parse_expression();

	switch (type) {
		case TOKEN_MINUS: compiler->emit_byte(OP_NEGATE); break;
		case TOKEN_BANG: compiler->emit_byte(OP_NOT); break;
		default: UNREACHABLE();
	}
}

void parse_number(bool can_assign) {
	double value = strtod(compiler->get_previous().start, nullptr);
	compiler->emit_constant(MAKE_NUMBER_VALUE(value));
}

static LitParseRule* get_rule(LitTokenType type) {
	return &rules[type];
}

void parse_binary(bool can_assign) {
	LitTokenType type = compiler->get_previous().type;

	LitParseRule* rule = get_rule(type);
	parse_precedence((LitPrecedence) (rule->precedence + 1));

	switch (type) {
		case TOKEN_BANG_EQUAL: compiler->emit_bytes(OP_EQUAL, OP_NOT); break;
		case TOKEN_EQUAL_EQUAL: compiler->emit_byte(OP_EQUAL); break;
		case TOKEN_GREATER: compiler->emit_byte(OP_GREATER); break;
		case TOKEN_GREATER_EQUAL: compiler->emit_bytes(OP_LESS, OP_NOT); break;
		case TOKEN_LESS: compiler->emit_byte(OP_LESS); break;
		case TOKEN_LESS_EQUAL: compiler->emit_bytes(OP_GREATER, OP_NOT); break;
		case TOKEN_PLUS: compiler->emit_byte(OP_ADD); break;
		case TOKEN_MINUS: compiler->emit_byte(OP_SUBTRACT); break;
		case TOKEN_STAR: compiler->emit_byte(OP_MULTIPLY); break;
		case TOKEN_SLASH: compiler->emit_byte(OP_DIVIDE); break;
		default: UNREACHABLE();
	}
}

void parse_literal(bool can_assign) {
	switch (compiler->get_previous().type) {
		case TOKEN_FALSE: compiler->emit_byte(OP_FALSE); break;
		case TOKEN_NIL: compiler->emit_byte(OP_NIL); break;
		case TOKEN_TRUE: compiler->emit_byte(OP_TRUE); break;
		default: UNREACHABLE();
	}
}

void parse_precedence(LitPrecedence precedence) {
	compiler->advance();
	LitParseFn prefixRule = get_rule(compiler->get_previous().type)->prefix;

	if (prefixRule == NULL) {
		compiler->error("Expect expression.");
		return;
	}

	bool can_assign = precedence <= PREC_ASSIGNMENT;
	prefixRule(can_assign);

	while (precedence <= get_rule(compiler->get_current().type)->precedence) {
		compiler->advance();
		get_rule(compiler->get_previous().type)->infix(can_assign);
	}

	/*
	if (can_assign && compiler->match(TOKEN_EQUAL)) {
		compiler->error("Invalid assignment target.");
		parse_expression();
	}*/
}

void parse_expression() {
	parse_precedence(PREC_ASSIGNMENT);
}

void parse_declaration() {
	parse_statement();
}

void parse_statement() {
	parse_expression();
	// emitByte(OP_POP);
}

bool LitCompiler::match(LitTokenType token) {
	if (current.type == token) {
		advance();
		return true;
	}

	return false;
}

bool LitCompiler::compile(LitChunk *cnk) {
	init_rules();

	compiler = this;
	chunk = cnk;
	had_error = false;
	panic_mode = false;

	advance();
	parse_expression();
	consume(TOKEN_EOF, "Expect end of expression.");
	emit_byte(OP_RETURN);

#ifdef DEBUG_PRINT_CODE
	if (!had_error) {
		lit_disassemble_chunk(compiler->get_chunk(), "code");
	}
#endif

	return !had_error;
}