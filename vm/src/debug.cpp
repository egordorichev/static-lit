#include <iostream>

#include "debug.hpp"

void lit_disassemble_chunk(LitChunk *chunk, const char *name) {
	std::cout << "== " << name << " ==\n";

	for (int i = 0; i < chunk->get_count();) {
		i = lit_disassemble_instruction(chunk, i);
	}
}

static int simple_instruction(const char *name, int offset) {
	std::cout << name << "\n";
	return offset + 1;
}

static int constant_instruction(const char *name, LitChunk *chunk, int offset) {
	uint8_t constant = chunk->get_code()[offset + 1];
	printf("%-16s %4d '", name, constant);
	lit_print_value(chunk->get_constants()->get(constant));
	printf("'\n");
}

int lit_disassemble_instruction(LitChunk *chunk, int i) {
	printf("%04d ", i);
	uint8_t instruction = chunk->get_code()[i];

	switch (instruction) {
		case OP_RETURN: return simple_instruction("OP_RETURN", i);
		case OP_CONSTANT: return constant_instruction("OP_CONSTANT", chunk, i);
		default: printf("Unknown opcode %d\n", instruction); return i + 1;
	}
}