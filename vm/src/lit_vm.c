#include <stdio.h>
#include "lit_vm.h"
#include "lit_compiler.h"
#include "lit.h"
#include "lit_debug.h"

static inline void reset_stack(LitVm *vm) {
	vm->stack_top = vm->stack;
}

void lit_init_vm(LitVm* vm) {
	reset_stack(vm);
}

void lit_free_vm(LitVm* vm) {
	if (vm->compiler != NULL) {
		lit_free_compiler(vm->compiler);
	}
}

void lit_push(LitVm* vm, LitValue value) {
	*vm->stack_top = value;
	vm->stack_top++;
}

LitValue lit_pop(LitVm* vm) {
	assert(vm->stack_top > vm->stack);

	vm->stack_top--;
	return *vm->stack_top;
}

LitCompiler compiler;

LitInterpretResult lit_execute(LitVm* vm, const char* code) {
	// FIXME: workaround
	if (vm->compiler == NULL) {
		lit_init_compiler(&compiler);

		compiler.vm = vm;
		vm->compiler = &compiler;
	}

	if (!lit_compile(vm->compiler, code)) {
		return INTERPRET_COMPILE_ERROR;
	}

	return INTERPRET_OK;
}

LitInterpretResult lit_interpret(LitVm* vm, LitChunk* chunk) {
	vm->chunk = chunk;
	vm->ip = chunk->code;

#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])

#define BINARY_OP(op) \
    do { \
      double b = lit_pop(vm); \
      double a = lit_pop(vm); \
      lit_push(vm, a op b); \
    } while (false)

	for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
		if (vm->stack != vm->stack_top) {
			for (LitValue* slot = vm->stack; slot < vm->stack_top; slot++) {
				printf("[ %s ]", lit_to_string(*slot));
			}

			printf("\n");
		}
		lit_disassemble_instruction(chunk, (int)(vm->ip - vm->chunk->code));
#endif

		switch (READ_BYTE()) {
			case OP_RETURN: lit_pop(vm); return INTERPRET_OK;
			case OP_CONSTANT: lit_push(vm, READ_CONSTANT()); break;
			case OP_PRINT: printf("%s\n", lit_to_string(lit_pop(vm))); break;
			case OP_NEGATE: lit_push(vm, -lit_pop(vm)); break;
			case OP_ADD: BINARY_OP(+); break;
			case OP_SUBTRACT: BINARY_OP(-); break;
			case OP_MULTIPLY: BINARY_OP(*); break;
			case OP_DIVIDE: BINARY_OP(/); break;
			default: UNREACHABLE();
		}
	}

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}