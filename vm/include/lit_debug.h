#ifndef LIT_DEBUG_H
#define LIT_DEBUG_H

#include "lit_chunk.h"

void lit_trace_chunk(LitVm* vm, LitChunk* chunk, const char* name);
int lit_disassemble_instruction(LitVm* vm, LitChunk* chunk, int offset);

// #define DEBUG_TRACE_EXECUTION
// #define DEBUG_PRINT_CODE
// #define DEBUG_TRACE_GC
// #define DEBUG_NO_EXECUTE

#endif
