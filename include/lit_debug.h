#ifndef LIT_DEBUG_H
#define LIT_DEBUG_H

#include <lit_mem_manager.h>

#include <vm/lit_chunk.h>
#include <compiler/lit_ast.h>

void lit_trace_chunk(LitMemManager* manager, LitChunk* chunk, const char* name);
uint64_t lit_disassemble_instruction(LitMemManager* manager, LitChunk* chunk, uint64_t offset);

#define DEBUG_TRACE_EXECUTION true
#define DEBUG_TRACE_CODE false
#define DEBUG_TRACE_GC false
#define DEBUG_TRACE_MEMORY_LEAKS true
#define DEBUG_NO_EXECUTE false

#endif