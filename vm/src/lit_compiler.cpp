#include "lit_vm.hpp"
#include "lit_object.hpp"
#include "lit_compiler.hpp"
#include "lit_common.hpp"

#ifdef DEBUG_PRINT_CODE
#include "lit_debug.hpp"
#endif

#include <cstring>

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
	rules[TOKEN_AND] = { nullptr, parse_binary, PREC_AND };
	rules[TOKEN_OR] = { nullptr, parse_binary, PREC_OR };
	rules[TOKEN_NUMBER] = { parse_number, nullptr, PREC_NONE };
	rules[TOKEN_NIL] = { parse_literal, nullptr, PREC_NONE };
	rules[TOKEN_TRUE] = { parse_literal, nullptr, PREC_NONE };
	rules[TOKEN_FALSE] = { parse_literal, nullptr, PREC_NONE };
	rules[TOKEN_STRING] = { parse_string, nullptr, PREC_NONE };
	rules[TOKEN_PRINT] = { nullptr, nullptr, PREC_NONE };
	rules[TOKEN_IDENTIFIER] = { parse_variable, NULL, PREC_NONE };
}

static uint8_t make_identifier_constant(LitToken name) {
	return compiler->make_constant(MAKE_OBJECT_VALUE(lit_copy_string(name.start, name.length)));
}

static void namedVariable(LitToken name, bool canAssign) {
	/*uint8_t getOp, setOp;
	int arg = resolve_local(compiler->get_current(), &name, false);

	if (arg != -1) {
		getOp = OP_GET_LOCAL;
		setOp = OP_SET_LOCAL;
	} else if ((arg = resolve_upvalue(compiler->get_current(), &name)) != -1) {
		getOp = OP_GET_UPVALUE;
		setOp = OP_SET_UPVALUE;
	} else {
		arg = make_identifier_constant(name);
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}

	if (canAssign && compiler->match(TOKEN_EQUAL)) {
		parse_expression();
		compiler->emit_bytes(setOp, (uint8_t) arg);
	} else {
		compiler->emit_bytes(getOp, (uint8_t) arg);
	}*/
}

void variable(bool canAssign) {
	namedVariable(compiler->get_previous(), canAssign);
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

#ifndef DEBUG
  fprintf(stderr, "[line %d] Error", token->line);
#else
  printf("[line %d] Error", token->line);
#endif

  if (token->type == TOKEN_EOF) {
#ifndef DEBUG
    fprintf(stderr, " at end");
#else
    printf(" at end");
#endif
  } else if (token->type != TOKEN_ERROR) {
#ifndef DEBUG
    fprintf(stderr, " at '%.*s'", token->length, token->start);
#else
    printf(" at '%.*s'", token->length, token->start);
#endif
  }

#ifndef DEBUG
  fprintf(stderr, ": %s\n", message);
#else
  printf(": %s\n", message);
#endif

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
		case TOKEN_AND: compiler->emit_byte(OP_AND); break;
		case TOKEN_OR: compiler->emit_byte(OP_OR); break;
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

  if (prefixRule == nullptr) {
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

static void addLocal(LitToken name) {
	if (compiler->get_local_count() == UINT8_COUNT) {
		compiler->error("Too many local variables in function.");
		return;
	}

	LitLocal* local = compiler->get_local(compiler->get_local_count());
	local->name = name;

	local->depth = -1;
	local->is_upvalue = false;
	compiler->add_local();
}

static bool are_identifiers_equal(LitToken* a, LitToken* b) {
	if (a->length != b->length) {
		return false;
	}

	return memcmp(a->start, b->start, a->length) == 0;
}

static void declare_variable() {
	if (compiler->get_depth() == 0) {
		return;
	}

	LitToken name = compiler->get_previous();

	for (int i = compiler->get_local_count() - 1; i >= 0; i--) {
		LitLocal* local = compiler->get_local(i);

		if (local->depth != -1 && local->depth < compiler->get_depth()) {
			break;
		}

		if (are_identifiers_equal(&name, &local->name)) {
			compiler->error("Variable with this name already declared in this scope.");
		}
	}

	addLocal(name);
}

static uint8_t parse_variable(const char* error) {
	compiler->consume(TOKEN_IDENTIFIER, error);

	if (compiler->get_depth() == 0) {
		return make_identifier_constant(compiler->get_previous());
	}

	declare_variable();
	return 0;
}

static void define_variable(uint8_t global) {
	if (compiler->get_depth() == 0) {
		compiler->emit_bytes(OP_DEFINE_GLOBAL, global);
	} else {
		compiler->define_local(compiler->get_local_count() - 1, compiler->get_depth());
	}
}

void parse_var_declaration() {
	uint8_t global = parse_variable("Expect variable name.");

	if (compiler->match(TOKEN_EQUAL)) {
		parse_expression();
	} else {
		compiler->emit_byte(OP_NIL);
	}

	define_variable(global);
}

void parse_declaration() {
	if (compiler->match(TOKEN_VAR)) {
		parse_var_declaration();
	} else {
		parse_statement();
	}
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
	} else if (compiler->match(TOKEN_WHILE)) {

	} else if (compiler->match(TOKEN_LEFT_BRACE)) {
		compiler->begin_scope();
		parse_block();
		compiler->end_scope();
	} else {
		parse_expression();
		compiler->emit_byte(OP_POP);
	}
}

void parse_variable(bool can_assign) {
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