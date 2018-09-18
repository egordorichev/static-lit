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

int lit_disassemble_instruction(LitChunk* chunk, int offset) {
	printf("%04d ", offset);
	uint8_t instruction = chunk->code[offset];

	switch (instruction) {
		case OP_RETURN: return simple_instruction("OP_RETURN", offset);
		case OP_CONSTANT: return constant_instruction("OP_CONSTANT", chunk, offset);
		default: printf("Unknown opcode %d\n", instruction); return offset + 1;
	}
}