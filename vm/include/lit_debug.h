#ifndef LIT_DEBUG_H
#define LIT_DEBUG_H

#include "lit_chunk.h"

void lit_trace_chunk(LitChunk* chunk, const char* name);
int lit_disassemble_instruction(LitChunk* chunk, int offset);

// #define DEBUG_TRACE_EXECUTION
// #define DEBUG_PRINT_CODE
// #define DEBUG_TRACE_GC

#endif