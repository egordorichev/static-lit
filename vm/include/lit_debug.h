#ifndef LIT_DEBUG_H
#define LIT_DEBUG_H

#include "lit_chunk.h"
#include "lit_ast.h"

void lit_trace_statement(LitVm* vm, LitStatement* statement, int depth);
void lit_trace_expression(LitVm* vm, LitExpression* expression, int depth);

void lit_trace_chunk(LitVm* vm, LitChunk* chunk, const char* name);
int lit_disassemble_instruction(LitVm* vm, LitChunk* chunk, int offset);

#define DEBUG_PRINT_AST true
#define DEBUG_TRACE_EXECUTION true
#define DEBUG_PRINT_CODE false
#define DEBUG_TRACE_GC false
#define DEBUG_NO_EXECUTE false

#endif