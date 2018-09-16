#ifndef LIT_HPP
#define LIT_HPP

#include "config.hpp"
#include "lit_chunk.hpp"
#include "vm.hpp"

class LitState {
	public:
		InterpretResult execute(const char* string);
};

#endif