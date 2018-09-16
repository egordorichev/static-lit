#ifndef LIT_VM_HPP
#define LIT_VM_HPP

#include "chunk.hpp"

#define STACK_MAX 256

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
} InterpretResult;

class LitVm {
	public:
		LitVm();
		~LitVm();

		InterpretResult interpret(LitChunk *cnk);
		void reset_stack();
		void push(LitValue value);
		LitValue pop();
	private:
		LitChunk *chunk;
		uint8_t *ip;
		LitValue stack[STACK_MAX];
		LitValue *stack_top;
};

#endif