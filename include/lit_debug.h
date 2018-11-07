#ifndef LIT_DEBUG_H
#define LIT_DEBUG_H

#include <lit_mem_manager.h>

#include <vm/lit_chunk.h>
#include <compiler/lit_ast.h>

void lit_trace_statement(LitMemManager* manager, LitStatement* statement, int depth);
void lit_trace_expression(LitMemManager* manager, LitExpression* expression, int depth);

void lit_trace_chunk(LitMemManager* manager, LitChunk* chunk, const char* name);
int lit_disassemble_instruction(LitMemManager* manager, LitChunk* chunk, int offset);

#define DEBUG_TRACE_AST false
#define DEBUG_TRACE_EXECUTION true
#define DEBUG_TRACE_CODE false
#define DEBUG_TRACE_GC false
#define DEBUG_NO_EXECUTE false

#endif