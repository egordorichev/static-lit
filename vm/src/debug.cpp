#include <iostream>

#include "debug.hpp"

void lit_disassemble_chunk(LitChunk *chunk, const char *name) {
	std::cout << "== " << name << " ==\n";

	for (int i = 0; i < chunk->count;) {
		i = lit_disassemble_instruction(chunk, i);
	}
}

static int simple_instruction(const char *name, int offset) {
	std::cout << name << "\n";
	return offset + 1;
}

int lit_disassemble_instruction(LitChunk *chunk, int i) {
	printf("%04d ", i);

	uint8_t instruction = chunk->code[i];
	switch (instruction) {
		case OP_RETURN: return simple_instruction("OP_RETURN", i);
		default: printf("Unknown opcode %d\n", instruction); return i + 1;
	}
}