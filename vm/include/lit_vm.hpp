#ifndef LIT_VM_HPP
#define LIT_VM_HPP

#include "lit_chunk.hpp"

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

		void reset_stack();
		void push(LitValue value);
		LitValue pop();
		LitValue peek(int depth);
		void runtime_error(const char* format, ...);

		InterpretResult run_chunk(LitChunk* cnk);
	private:
    LitChunk* chunk;
    uint8_t* ip;
    LitValue stack[STACK_MAX];
    LitValue* stack_top;
};

#endif