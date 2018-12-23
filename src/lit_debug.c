#include <stdio.h>

#include <lit_debug.h>

#include <vm/lit_object.h>
#include <vm/lit_value.h>

void lit_trace_chunk(LitMemManager* manager, LitChunk* chunk, const char* name) {
	printf("== %s ==\n", name);

	for (uint64_t i = 0; i < chunk->count;) {
		i = lit_disassemble_instruction(manager, chunk, i);
	}
}

static uint64_t simple_instruction(const char* name, uint64_t offset) {
	printf("%-16s\n", name);
	return offset + 1;
}

static uint64_t binary_instruction(LitMemManager* manager, const char* name, uint64_t offset, LitChunk* chunk, const char* op) {
	printf("%-16s %i = %i %s %i\n", name, chunk->code[offset + 1], chunk->code[offset + 2], op, chunk->code[offset + 3]);
	return offset + 4;
}

static uint64_t constant_instruction(LitMemManager* manager, const char* name, uint64_t offset, LitChunk* chunk) {
	uint8_t constant = chunk->code[offset + 2];
	printf("%-16s %d = c%d (%s)\n", name, chunk->code[offset + 1], constant, lit_to_string((LitVm*) manager, chunk->constants.values[constant]));
	return offset + 3;
}

static uint64_t long_constant_instruction(LitMemManager* manager, const char* name, uint64_t offset, LitChunk* chunk) {
	uint8_t constant = chunk->code[offset + 2];
	printf("%-16s %d = c%d (%s)\n", name, chunk->code[offset + 1], (constant << 8) | chunk->code[offset + 3], lit_to_string((LitVm*) manager, chunk->constants.values[constant]));
	return offset + 3;
}

uint64_t lit_disassemble_instruction(LitMemManager* manager, LitChunk* chunk, uint64_t offset) {
	printf("%4lu ", offset);
	uint8_t instruction = chunk->code[offset];

#define CASE_CODE(name) case OP_##name:

	switch (instruction) {
		CASE_CODE(EXIT) return simple_instruction("OP_EXIT", offset);
		CASE_CODE(RETURN) return simple_instruction("OP_RETURN", offset) + 1;
		CASE_CODE(CONSTANT) return constant_instruction(manager, "OP_CONSTANT", offset, chunk);
		CASE_CODE(CONSTANT_LONG) return long_constant_instruction(manager, "OP_CONSTANT", offset, chunk);
		CASE_CODE(ADD) return binary_instruction(manager, "OP_ADD", offset, chunk, "+");
		CASE_CODE(SUBTRACT) return binary_instruction(manager, "OP_SUBTRACT", offset, chunk, "-");
		CASE_CODE(MULTIPLY) return binary_instruction(manager, "OP_MULTIPLY", offset, chunk, "*");
		CASE_CODE(DIVIDE) return binary_instruction(manager, "OP_DIVIDE", offset, chunk, "/");
		CASE_CODE(MODULO) return binary_instruction(manager, "OP_MODULO", offset, chunk, "%");
		CASE_CODE(POWER) return binary_instruction(manager, "OP_POWER", offset, chunk, "^");
		CASE_CODE(ROOT) return binary_instruction(manager, "OP_ROOT", offset, chunk, "#");
		CASE_CODE(DEFINE_FUNCTION) {
			printf("%-16s %i = %s\n", "OP_DEFINE_FUNCTION", chunk->code[offset + 1], lit_to_string((LitVm*) manager, chunk->constants.values[chunk->code[offset + 2]]));
			return offset + 2;
		}
		CASE_CODE(TRUE) {
			printf("%-16s\n", "OP_TRUE");
			return offset + 1;
		}
		CASE_CODE(FALSE) {
			printf("%-16s\n", "OP_FALSE");
			return offset + 1;
		}
		CASE_CODE(NIL) {
			printf("%-16s\n", "OP_NIL");
			return offset + 1;
		}
		CASE_CODE(NOT) {
			printf("%-16s %i = !%i\n", "OP_NOT", chunk->code[offset + 1], chunk->code[offset + 2]);
			return offset + 3;
		}
		default: printf("Unknown code %i\n", instruction); return offset + 1;
	}

#undef CASE_CODE

	/*
	switch (instruction) {
		case OP_RETURN: return simple_instruction("OP_RETURN", offset);
		case OP_ADD: return simple_instruction("OP_ADD", offset);
		case OP_POP: return simple_instruction("OP_POP", offset);
		case OP_ROOT: return simple_instruction("OP_ROOT", offset);
		case OP_SQUARE: return simple_instruction("OP_SQUARE", offset);
		case OP_IS: return simple_instruction("OP_IS", offset);
		case OP_SUBTRACT: return simple_instruction("OP_SUBTRACT", offset);
		case OP_MULTIPLY: return simple_instruction("OP_MULTIPLY", offset);
		case OP_DIVIDE: return simple_instruction("OP_DIVIDE", offset);
		case OP_STATIC_INIT: return simple_instruction("OP_STATIC_INIT", offset);
		case OP_NEGATE: return simple_instruction("OP_NEGATE", offset);
		case OP_NOT: return simple_instruction("OP_NOT", offset);
		case OP_NIL: return simple_instruction("OP_NIL", offset);
		case OP_TRUE: return simple_instruction("OP_TRUE", offset);
		case OP_FALSE: return simple_instruction("OP_FALSE", offset);
		case OP_EQUAL: return simple_instruction("OP_EQUAL", offset);
		case OP_LESS: return simple_instruction("OP_LESS", offset);
		case OP_GREATER: return simple_instruction("OP_GREATER", offset);
		case OP_NOT_EQUAL: return simple_instruction("OP_NOT_EQUAL", offset);
		case OP_GREATER_EQUAL: return simple_instruction("OP_GREATER_EQUAL", offset);
		case OP_LESS_EQUAL: return simple_instruction("OP_LESS_EQUAL", offset);
		case OP_CALL: return simple_instruction("OP_CALL", offset) + 1;
		case OP_DEFINE_GLOBAL: return constant_instruction(manager, "OP_DEFINE_GLOBAL", chunk, offset);
		case OP_GET_GLOBAL: return constant_instruction(manager, "OP_GET_GLOBAL", chunk, offset);
		case OP_SET_GLOBAL: return constant_instruction(manager, "OP_SET_GLOBAL", chunk, offset);
		case OP_GET_LOCAL: return byte_instruction("OP_GET_LOCAL", chunk, offset);
		case OP_SET_LOCAL: return byte_instruction("OP_SET_LOCAL", chunk,offset);
		case OP_GET_UPVALUE: return byte_instruction("OP_GET_UPVALUE", chunk, offset);
		case OP_SET_UPVALUE: return byte_instruction("OP_SET_UPVALUE", chunk, offset);
		case OP_CONSTANT: return constant_instruction(manager, "OP_CONSTANT", chunk, offset);
		case OP_JUMP: return jump_instruction("OP_JUMP", 1, chunk, offset);
		case OP_JUMP_IF_FALSE: return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
		case OP_LOOP: return jump_instruction("OP_LOOP", -1, chunk, offset);
		case OP_CLASS: return constant_instruction(manager, "OP_CLASS", chunk, offset);
		case OP_SUBCLASS: return constant_instruction(manager, "OP_SUBCLASS", chunk, offset);
		case OP_METHOD: return constant_instruction(manager, "OP_METHOD", chunk, offset);
		case OP_GET_FIELD: return constant_instruction(manager, "OP_GET_FIELD", chunk, offset);
		case OP_SET_FIELD: return constant_instruction(manager, "OP_SET_FIELD", chunk, offset);
		case OP_DEFINE_FIELD: return constant_instruction(manager, "OP_DEFINE_FIELD", chunk, offset);
		case OP_DEFINE_METHOD: return constant_instruction(manager, "OP_DEFINE_METHOD", chunk, offset);
		case OP_DEFINE_STATIC_FIELD: return constant_instruction(manager, "OP_DEFINE_STATIC_FIELD", chunk, offset);
		case OP_DEFINE_STATIC_METHOD: return constant_instruction(manager, "OP_DEFINE_STATIC_METHOD", chunk, offset);
		case OP_INVOKE: return constant_instruction(manager, "OP_INVOKE", chunk, offset) + 2;
		case OP_SUPER: return constant_instruction(manager, "OP_SUPER", chunk, offset);
		case OP_CLOSURE: {
			offset++;
			uint8_t constant = chunk->code[offset++];
			printf("%-16s %4d %s\n", "OP_CLOSURE", constant, lit_to_string((LitVm*) (manager), chunk->constants.values[constant]));

			LitFunction* function = AS_FUNCTION(chunk->constants.values[constant]);

			for (int j = 0; j < function->upvalue_count; j++) {
				int isLocal = chunk->code[offset++];
				int index = chunk->code[offset++];

				printf("%lu   |                     %s %d\n", offset - 2, isLocal ? "local" : "upvalue", index);
			}

			return offset;
		}
		default: printf("Unknown opcode %i\n", instruction); return offset + 1;
	}*/
}