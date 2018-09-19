#include <stdio.h>

#include "lit_debug.h"
#include "lit_value.h"

void lit_trace_chunk(LitChunk* chunk, const char* name) {
	printf("== %s ==\n", name);

	for (int i = 0; i < chunk->count;) {
		i = lit_disassemble_instruction(chunk, i);
	}
}

static int simple_instruction(const char* name, int offset) {
	printf("%s\n", name);
	return offset + 1;
}

static int constant_instruction(const char* name, LitChunk* chunk, int offset) {
	uint8_t constant = chunk->code[offset + 1];
	printf("%-16s %4d '%s'\n", name, constant, lit_to_string(chunk->constants.values[constant]));
	return offset + 2;
}

static int byte_instruction(const char* name, LitChunk* chunk, int offset) {
	uint8_t slot = chunk->code[offset + 1];
	printf("%-16s %4d\n", name, slot);
	return offset + 2;
}

int lit_disassemble_instruction(LitChunk* chunk, int offset) {
	printf("%04d ", offset);

	if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
		printf("| ");
	} else {
		printf("%d ", chunk->lines[offset]);
	}

	uint8_t instruction = chunk->code[offset];

	switch (instruction) {
		case OP_RETURN: return simple_instruction("OP_RETURN", offset);
		case OP_ADD: return simple_instruction("OP_ADD", offset);
		case OP_POP: return simple_instruction("OP_POP", offset);
		case OP_SUBTRACT: return simple_instruction("OP_SUBTRACT", offset);
		case OP_MULTIPLY: return simple_instruction("OP_MULTIPLY", offset);
		case OP_DIVIDE: return simple_instruction("OP_DIVIDE", offset);
		case OP_PRINT: return simple_instruction("OP_PRINT", offset);
		case OP_NEGATE: return simple_instruction("OP_NEGATE", offset);
		case OP_NOT: return simple_instruction("OP_NOT", offset);
		case OP_NIL: return simple_instruction("OP_NIL", offset);
		case OP_TRUE: return simple_instruction("OP_TRUE", offset);
		case OP_FALSE: return simple_instruction("OP_FALSE", offset);
		case OP_EQUAL: return simple_instruction("OP_EQUAL", offset);
		case OP_LESS: return simple_instruction("OP_LESS", offset);
		case OP_GREATER: return simple_instruction("OP_GREATER", offset);
		case OP_NOT_EQUAL: return simple_instruction("OP_NOT_EQUAL", offset);
		case OP_GREATER_EQUAL: return simple_instruction("OP_GREATER_EQUAL", offset);
		case OP_LESS_EQUAL: return simple_instruction("OP_LESS_EQUAL", offset);
		case OP_DEFINE_GLOBAL: return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
		case OP_GET_GLOBAL: return constant_instruction("OP_GET_GLOBAL", chunk, offset);
		case OP_SET_GLOBAL: return constant_instruction("OP_SET_GLOBAL", chunk, offset);
		case OP_GET_LOCAL: return byte_instruction("OP_GET_LOCAL", chunk, offset);
		case OP_SET_LOCAL: return byte_instruction("OP_SET_LOCAL", chunk,offset);
		case OP_GET_UPVALUE: return byte_instruction("OP_GET_UPVALUE", chunk, offset);
		case OP_SET_UPVALUE: return byte_instruction("OP_SET_UPVALUE", chunk, offset);
		case OP_CONSTANT: return constant_instruction("OP_CONSTANT", chunk, offset);
		default: printf("Unknown opcode %i\n", instruction); return offset + 1;
	}
}