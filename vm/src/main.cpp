#include <iostream>

#include "lit.hpp"
#include "debug.hpp"
#include "vm.hpp"

int main(int argc, char **argv) {
	LitVm vm;
	LitChunk chunk;

	int constant = chunk.add_constant(1.2);

	chunk.write(OP_CONSTANT, 0);
	chunk.write(constant, 0);
	chunk.write(OP_RETURN, 0);

	InterpretResult result = vm.interpret(&chunk);

	if (result == INTERPRET_OK) {
		std::cout << "INTERPRET_OK\n";
	}

	return 0;
}