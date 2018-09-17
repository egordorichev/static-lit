#include <iostream>

#include "lit_debug.hpp"

void lit_disassemble_chunk(LitChunk* chunk, const char* name) {
  std::cout << "== " << name << " ==\n";

  for (int i = 0; i < chunk->get_count();) {
    i = lit_disassemble_instruction(chunk, i);
  }
}

static int simple_instruction(const char* name, int offset) {
  std::cout << name << "\n";
  return offset + 1;
}

static int constant_instruction(const char* name, LitChunk* chunk, int offset) {
  uint8_t constant = chunk->get_code()[offset + 1];

  printf("%-16s %4d '", name, constant);
  lit_print_value(chunk->get_constants()->get(constant));
  printf("'\n");

  return offset + 2;
}

int lit_disassemble_instruction(LitChunk* chunk, int i) {
  printf("%04d ", i);
  uint8_t instruction = chunk->get_code()[i];

  if (i > 0 && chunk->get_line(i) == chunk->get_line(i - 1)) {
    printf("| ");
  } else {
    printf("%d ", chunk->get_line(i));
  }

	switch (instruction) {
		case OP_RETURN: return simple_instruction("OP_RETURN", i);
		case OP_CONSTANT: return constant_instruction("OP_CONSTANT", chunk, i);
		case OP_NEGATE: return simple_instruction("OP_NEGATE", i);
		case OP_ADD: return simple_instruction("OP_ADD", i);
		case OP_SUBTRACT: return simple_instruction("OP_SUBTRACT", i);
		case OP_DIVIDE: return simple_instruction("OP_DIVIDE", i);
		case OP_MULTIPLY: return simple_instruction("OP_MULTIPLY", i);
		case OP_NIL: return simple_instruction("OP_NIL", i);
		case OP_TRUE: return simple_instruction("OP_TRUE", i);
		case OP_FALSE: return simple_instruction("OP_FALSE", i);
		case OP_NOT: return simple_instruction("OP_NOT", i);
		case OP_EQUAL: return simple_instruction("OP_EQUAL", i);
		case OP_GREATER: return simple_instruction("OP_GREATER", i);
		case OP_LESS: return simple_instruction("OP_LESS", i);
		default: printf("Unknown opcode %d\n", instruction); return i + 1;
	}
}