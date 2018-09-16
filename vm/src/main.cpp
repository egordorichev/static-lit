#include <iostream>

#include "lit.hpp"
#include "debug.hpp"

int main(int argc, char **argv) {
	LitChunk chunk;

	lit_init_chunk(&chunk);
	lit_write_chunk(&chunk, OP_RETURN);

	lit_disassemble_chunk(&chunk, "test chunk");

	lit_free_chunk(&chunk);

	return 0;
}