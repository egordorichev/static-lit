#include <cstdio>
#include "vm.hpp"
#include "debug.hpp"

LitVm::LitVm() {
	reset_stack();
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
		if (stack != stack_top) {
			printf("     | ");

			for (LitValue *slot = stack; slot < stack_top; slot++) {
				printf("[ ");
				lit_print_value(*slot);
				printf(" ]");
			}

			printf("\n");
		}

		lit_disassemble_instruction(chunk, (int)(ip - chunk->get_code() - 1));
#endif

		switch (instruction) {
			case OP_RETURN: {
				lit_print_value(pop());
				printf("\n");
				return INTERPRET_OK;
			}
			case OP_CONSTANT: {
				push(READ_CONSTANT());
				break;
			}
			case OP_NEGATE: {
				push(-pop());
			}
		}
	}

#undef READ_CONSTANT
#undef READ_BYTE
}

void LitVm::reset_stack() {
	stack_top = stack;
}

void LitVm::push(LitValue value) {
	*stack_top = value;
	stack_top ++;
}

LitValue LitVm::pop() {
	stack_top --;
	return *stack_top;
}