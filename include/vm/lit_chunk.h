#ifndef LIT_CHUNK_H
#define LIT_CHUNK_H

#include <lit_common.h>
#include <lit_predefines.h>
#include <vm/lit_value.h>
#include <vm/lit_memory.h>
#include <util/lit_array.h>

DECLARE_ARRAY(LitArray, LitValue, array)

typedef enum {
	OP_RETURN = 0,
	OP_CONSTANT = 1,
	OP_STATIC_INIT = 2,
	OP_NEGATE = 3,
	OP_ADD = 4,
	OP_SUBTRACT = 5,
	OP_MULTIPLY = 6,
	OP_DIVIDE = 7,
	OP_POP = 8,
	OP_NOT = 9,
	OP_NIL = 10,
	OP_TRUE = 11,
	OP_FALSE = 12,
	OP_EQUAL = 13,
	OP_GREATER = 14,
	OP_LESS = 15,
	// We could make those a combination of op codes
	// Yet it will be a bit slower
	OP_GREATER_EQUAL = 16,
	OP_LESS_EQUAL = 17,
	OP_NOT_EQUAL = 18,
	OP_CLOSE_UPVALUE = 19,
	OP_DEFINE_GLOBAL = 20,
	OP_GET_GLOBAL = 21,
	OP_SET_GLOBAL = 22,
	OP_GET_LOCAL = 23,
	OP_SET_LOCAL = 24,
	OP_GET_UPVALUE = 25,
	OP_SET_UPVALUE = 26,
	OP_JUMP = 27,
	OP_JUMP_IF_FALSE = 28,
	OP_LOOP = 29,
	OP_CLOSURE = 30,
	OP_SUBCLASS = 31,
	OP_CLASS = 32,
	OP_METHOD = 33,
	OP_GET_FIELD = 34,
	OP_SET_FIELD = 35,
	OP_INVOKE = 36,
	OP_CALL = 37,
	OP_DEFINE_FIELD = 38,
	OP_DEFINE_METHOD = 39,
	OP_SUPER = 40,
	OP_DEFINE_STATIC_FIELD = 41,
	OP_DEFINE_STATIC_METHOD = 42,
	OP_POWER = 43,
	OP_SQUARE = 44,
	OP_ROOT = 45,
	OP_IS = 46,
	OP_MODULO = 47,
	OP_FLOOR = 48,

	OP_TOTAL = 49
} LitOpCode;

typedef struct {
	uint64_t count;
	uint64_t capacity;
	uint8_t* code;
	uint64_t* lines;
	uint64_t line_count;
	uint64_t line_capacity;

	LitArray constants;
} LitChunk;

void lit_init_chunk(LitChunk* chunk);
void lit_free_chunk(LitMemManager* manager, LitChunk* chunk);

void lit_chunk_write(LitMemManager* manager, LitChunk* chunk, uint8_t byte, uint64_t line);
int lit_chunk_add_constant(LitMemManager* manager, LitChunk* chunk, LitValue constant);
uint64_t lit_chunk_get_line(LitChunk* chunk, uint64_t offset);

#endif