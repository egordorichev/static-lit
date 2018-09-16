#ifndef LIT_DEBUG_HPP
#define LIT_DEBUG_HPP

#include "chunk.hpp"

void lit_disassemble_chunk(LitChunk *chunk, const char *name);
int lit_disassemble_instruction(LitChunk *chunk, int i);

#endif