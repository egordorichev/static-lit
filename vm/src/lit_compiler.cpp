#include <lit_vm.hpp>
#include "lit_object.hpp"
#include "lit_compiler.hpp"
#include "lit_common.hpp"

#ifdef DEBUG_PRINT_CODE

#include "lit_debug.hpp"

#endif

static LitCompiler* compiler;

static LitParseRule rules[TOKEN_EOF + 1];
bool inited_rules = false;

static void init_rules() {
	if (inited_rules) {
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
	rules[TOKEN_NUMBER] = { parse_number, nullptr, PREC_NONE };
	rules[TOKEN_NIL] = { parse_literal, nullptr, PREC_NONE };
	rules[TOKEN_TRUE] = { parse_literal, nullptr, PREC_NONE };
	rules[TOKEN_FALSE] = { parse_literal, nullptr, PREC_NONE };
	rules[TOKEN_STRING] = { parse_string, nullptr, PREC_NONE };
	rules[TOKEN_PRINT] = { nullptr, nullptr, PREC_NONE };
}

void parse_print() {
	parse_expression();
	compiler->emit_byte(OP_PRINT);
}

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

void LitCompiler::error_at_current(const char* message) {
  error_at(&current, message);
}

void LitCompiler::error(const char* message) {
  error_at(&previous, message);
}

void LitCompiler::error_at(LitToken* token, const char* message) {
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
  chunk->write(byte, previous.line);
}

void LitCompiler::emit_bytes(uint8_t a, uint8_t b) {
  chunk->write(a, previous.line);
  chunk->write(b, previous.line);
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

  switch (type) {
    case TOKEN_MINUS:
      compiler->emit_byte(OP_NEGATE);
      break;
    case TOKEN_BANG:
      compiler->emit_byte(OP_NOT);
      break;
    default:
    UNREACHABLE();
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
    case TOKEN_GREATER:compiler->emit_byte(OP_GREATER); break;
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

void parse_string(bool can_assign) {
  compiler->emit_constant(
    MAKE_OBJECT_VALUE(lit_copy_string(compiler->get_previous().start + 1, compiler->get_previous().length - 2)));
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

  while(precedence <= get_rule(compiler->get_current().type)->precedence) {
    compiler->advance();
    get_rule(compiler->get_previous().type)->infix(can_assign);
  }

  if (can_assign && compiler->match(TOKEN_EQUAL)) {
    compiler->error("Invalid assignment target.");
    parse_expression();
  }
}

void parse_expression() {
  parse_precedence(PREC_ASSIGNMENT);
}

static int emit_jump(uint8_t instruction) {
	compiler->emit_byte(instruction);
	compiler->emit_byte(0xff);
	compiler->emit_byte(0xff);

	return lit_get_active_vm()->get_chunk()->get_count() - 2;
}

static void patch_jump(int offset) {
	LitChunk* chunk = lit_get_active_vm()->get_chunk();

	int jump = chunk->get_count() - offset - 2;

	if (jump > UINT16_MAX) {
		compiler->error("Too much code to jump over.");
	}

	chunk->get_code()[offset] = (jump >> 8) & 0xff;
	chunk->get_code()[offset + 1] = jump & 0xff;
}

void parse_if() {
	compiler->consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	parse_expression();
	compiler->consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	int elseJump = emit_jump(OP_JUMP_IF_FALSE);

	compiler->emit_byte(OP_POP);
	parse_statement();
	int endJump = emit_jump(OP_JUMP);

	patch_jump(elseJump);
	compiler->emit_byte(OP_POP);

	if (compiler->match(TOKEN_ELSE)) {
		parse_statement();
	}

	patch_jump(endJump);
}

void parse_declaration() {
  parse_statement();
}

void parse_block() {
	while (!compiler->check(TOKEN_RIGHT_BRACE) && !compiler->check(TOKEN_EOF)) {
		parse_declaration();
	}

	compiler->consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

void parse_statement() {
	if (compiler->match(TOKEN_IF)) {
		parse_if();
	} else if (compiler->match(TOKEN_PRINT)) {
		parse_print();
	} else if (compiler->match(TOKEN_LEFT_BRACE)) {
		compiler->begin_scope();
		parse_block();
		compiler->end_scope();
	} else {
		parse_expression();
		compiler->emit_byte(OP_POP);
	}
}

bool LitCompiler::match(LitTokenType token) {
	if (compiler->check(token)) {
		advance();
		return true;
	}

  return false;
}

bool LitCompiler::compile(LitChunk* cnk) {
  init_rules();

  compiler = this;
  chunk = cnk;
  had_error = false;
  panic_mode = false;

	advance();

	if (!match(TOKEN_EOF)) {
		do {
			parse_declaration();
		} while (!match(TOKEN_EOF));
	}

	emit_byte(OP_RETURN);

#ifdef DEBUG_PRINT_CODE
  if (!had_error) {
    lit_disassemble_chunk(compiler->get_chunk(), "code");
  }
#endif

  return !had_error;
}