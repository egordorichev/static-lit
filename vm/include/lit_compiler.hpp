#ifndef LIT_COMPILER_HPP
#define LIT_COMPILER_HPP

#include "lit_lexer.hpp"
#include "lit_chunk.hpp"

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! - +
  PREC_CALL,        // . () []
  PREC_PRIMARY
} LitPrecedence;

typedef void (* LitParseFn)(bool);

typedef struct {
  LitParseFn prefix;
  LitParseFn infix;
  LitPrecedence precedence;
} LitParseRule;

typedef struct {
		LitToken name;
		int depth;
		bool is_upvalue;
} LitLocal;

typedef struct {
		uint8_t index;
		bool is_local;
} LitUpvalue;

class LitCompiler {
	private:
		int depth;
		int local_count;
		LitChunk *chunk;
		LitLexer *lexer;
		LitToken current;
		LitToken previous;
		bool had_error;
		bool panic_mode;
		LitLocal locals[UINT8_COUNT];
		LitUpvalue upvalues[UINT8_COUNT];

	public:
		void set_lexer(LitLexer *lexer) { this->lexer = lexer; };
		bool compile(LitChunk *cnk);
		void add_local() { local_count ++; }
		int get_local_count() { return local_count; }
		LitLocal* get_local(int i) { return &locals[i]; }
		LitUpvalue* get_upvalue(int i) { return &upvalues[i]; }
		void define_local(int i, int w) { locals[i].depth = w; }

		LitChunk *get_chunk() { return chunk; }
		LitLexer *get_lexer() { return lexer; }
		LitToken get_current() { return current; }
		LitToken get_previous() { return previous; }

		void advance();
		void error_at_current(const char* message);
		void error(const char* message);
		void error_at(LitToken* token, const char* message);
		void consume(LitTokenType type, const char* message);
		bool match(LitTokenType token);
		void emit_byte(uint8_t byte);
		void emit_bytes(uint8_t a, uint8_t b);
		void emit_constant(LitValue value);
		uint8_t make_constant(LitValue value);

		void begin_scope() { depth ++; }
		void end_scope() { depth --; }

		int get_depth() { return depth; }
		bool check(LitTokenType type) { return current.type == type;	}
};

void parse_grouping(bool can_assign);
void parse_unary(bool can_assign);
void parse_binary(bool can_assign);
void parse_number(bool can_assign);
void parse_literal(bool can_assign);
void parse_string(bool can_assign);
void parse_variable(bool can_assign);
void parse_precedence(LitPrecedence precedence);
void parse_expression();
void parse_declaration();
void parse_statement();

#endif