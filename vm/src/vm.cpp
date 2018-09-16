#include <cstdio>
#include "vm.hpp"
#include "debug.hpp"

LitVm::LitVm() {

}

LitVm::~LitVm() {

}

InterpretResult LitVm::interpret(LitChunk *cnk) {
	chunk = cnk;
	ip = cnk->get_code();

#define READ_BYTE() (*ip++)
#define READ_CONSTANT() (chunk->get_constants()->get(READ_BYTE()))

	uint8_t instruction;

	for (;;) {
		instruction = READ_BYTE();

#ifdef DEBUG_TRACE_EXECUTION
		lit_disassemble_instruction(chunk, (int)(ip - chunk->get_code() - 1));
#endif

		switch (instruction) {
			case OP_RETURN: return INTERPRET_OK;
			case OP_CONSTANT: {
				LitValue constant = READ_CONSTANT();
				break;
			}
		}
	}

#undef READ_CONSTANT
#undef READ_BYTE
}