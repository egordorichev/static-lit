#ifndef LIT_CHUNK_H
#define LIT_CHUNK_H

#include <lit_common.h>
#include <lit_predefines.h>
#include <vm/lit_value.h>
#include <vm/lit_memory.h>
#include <util/lit_array.h>

typedef enum {
	#define OPCODE(name) OP_##name,
	#include <vm/lit_opcode.h>
	#undef OPCODE
} LitOpCode;

DECLARE_ARRAY(LitArray, LitValue, array)
void lit_gray_array(LitVm* vm, LitArray* array);

typedef struct {
	uint64_t count;
	uint64_t capacity;
	uint8_t* code;
	uint64_t* lines;
	uint64_t line_count;
	uint64_t line_capacity;

	LitArray constants;
} LitChunk;

void lit_init_chunk(LitChunk* chunk);
void lit_free_chunk(LitMemManager* manager, LitChunk* chunk);

void lit_chunk_write(LitMemManager* manager, LitChunk* chunk, uint8_t byte, uint64_t line);
int lit_chunk_add_constant(LitMemManager* manager, LitChunk* chunk, LitValue constant);
uint64_t lit_chunk_get_line(LitChunk* chunk, uint64_t offset);

#endif