#ifndef LIT_VM_HPP
#define LIT_VM_HPP

#include "chunk.hpp"

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
	private:
		LitChunk *chunk;
		uint8_t *ip;
};

#endif