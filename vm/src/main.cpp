#include <iostream>

#include "lit.hpp"
#include "debug.hpp"

int main(int argc, char **argv) {
	LitChunk chunk;

	int constant = chunk.add_constant(1.2);

	chunk.write(OP_CONSTANT);
	chunk.write(constant);
	chunk.write(OP_RETURN);

	lit_disassemble_chunk(&chunk, "test chunk");

	return 0;
}