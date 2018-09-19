#include <stdio.h>
#include <zconf.h>
#include <string.h>

#include "lit_vm.h"
#include "lit_compiler.h"
#include "lit.h"
#include "lit_debug.h"
#include "lit_memory.h"
#include "lit_object.h"

static inline void reset_stack(LitVm *vm) {
	vm->stack_top = vm->stack;
}

void lit_init_vm(LitVm* vm) {
	reset_stack(vm);

	lit_init_table(&vm->strings);
	lit_init_table(&vm->globals);

	vm->compiler = NULL;

	vm->bytes_allocated = 0;
	vm->next_gc = 1024 * 1024;
	vm->objects = NULL;

	vm->gray_capacity = 0;
	vm->gray_count = 0;
	vm->gray_stack = NULL;
}

void lit_free_vm(LitVm* vm) {
	lit_free_table(vm, &vm->strings);
	lit_free_table(vm, &vm->globals);

	if (vm->compiler != NULL) {
		lit_free_compiler(vm->compiler);
	}

	lit_free_objects(vm);
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

LitValue lit_peek(LitVm* vm, int depth) {
	return vm->stack_top[-1 - depth];
}

LitCompiler compiler;

static void runtime_error(LitVm* vm, const char* format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	size_t instruction = vm->ip - vm->chunk->code;
	fprintf(stderr, "[line %d] in script\n", vm->chunk->lines[instruction]);

	reset_stack(vm);
}

LitInterpretResult lit_execute(LitVm* vm, const char* code) {
	// FIXME: workaround
	if (vm->compiler == NULL) {
		lit_init_compiler(&compiler);

		compiler.vm = vm;
		vm->compiler = &compiler;
	}

	LitChunk chunk;
	lit_init_chunk(&chunk);

	if (!lit_compile(vm->compiler, &chunk, code)) {
		lit_free_chunk(vm, &chunk);
		return INTERPRET_COMPILE_ERROR;
	}

	lit_interpret(vm, &chunk);
	lit_free_chunk(vm, &chunk);

	return INTERPRET_OK;
}

LitInterpretResult lit_interpret(LitVm* vm, LitChunk* chunk) {
	vm->chunk = chunk;
	vm->ip = chunk->code;

#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())

#define BINARY_OP(type, op) \
   do { \
    if (!IS_NUMBER(lit_peek(vm, 0)) || !IS_NUMBER(lit_peek(vm, 1))) { \
      runtime_error(vm, "Operands must be numbers"); \
      return INTERPRET_RUNTIME_ERROR; \
    } \
    \
    double b = AS_NUMBER(lit_pop(vm)); \
    double a = AS_NUMBER(lit_pop(vm)); \
    lit_push(vm, type(a op b)); \
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
			case OP_RETURN: /*lit_pop(vm);*/ return INTERPRET_OK;
			case OP_CONSTANT: lit_push(vm, READ_CONSTANT()); break;
			case OP_PRINT: printf("%s\n", lit_to_string(lit_pop(vm))); break;
			case OP_NEGATE: {
				if (!IS_NUMBER(lit_peek(vm, 0))) {
					runtime_error(vm, "Operand must be a number");
					return INTERPRET_RUNTIME_ERROR;
				}

				lit_push(vm, MAKE_NUMBER_VALUE(-AS_NUMBER(lit_pop(vm))));
				break;
			}
			case OP_ADD: {
				LitValue b = lit_pop(vm);
				LitValue a = lit_pop(vm);

				if (IS_NUMBER(a) && IS_NUMBER(b)) {
					lit_push(vm, MAKE_NUMBER_VALUE(AS_NUMBER(a) + AS_NUMBER(b)));
				} else {
					char *as = lit_to_string(a);
					char *bs = lit_to_string(b);

					int al = strlen(as);
					int bl = strlen(bs);
					int length = al + bl;

					char* chars = ALLOCATE(vm, char, length + 1);

					memcpy(chars, as, al);
					memcpy(chars + al, bs, bl);
					chars[length] = '\0';

					lit_push(vm, MAKE_OBJECT_VALUE(lit_make_string(vm, chars, length)));
				}
				break;
			}
			case OP_SUBTRACT: BINARY_OP(MAKE_NUMBER_VALUE, -); break;
			case OP_MULTIPLY: BINARY_OP(MAKE_NUMBER_VALUE, *); break;
			case OP_DIVIDE: BINARY_OP(MAKE_NUMBER_VALUE, /); break;
			case OP_LESS: BINARY_OP(MAKE_BOOL_VALUE, <); break;
			case OP_LESS_EQUAL: BINARY_OP(MAKE_BOOL_VALUE, <=); break;
			case OP_GREATER: BINARY_OP(MAKE_BOOL_VALUE, >); break;
			case OP_GREATER_EQUAL: BINARY_OP(MAKE_BOOL_VALUE, >=); break;
			case OP_POP: lit_pop(vm); break;
			case OP_NOT: lit_push(vm, MAKE_BOOL_VALUE(lit_is_false(lit_pop(vm)))); break;
			case OP_NIL: lit_push(vm, NIL_VALUE); break;
			case OP_TRUE: lit_push(vm, TRUE_VALUE); break;
			case OP_FALSE: lit_push(vm, FALSE_VALUE); break;
			case OP_EQUAL: lit_push(vm, MAKE_BOOL_VALUE(lit_are_values_equal(lit_pop(vm), lit_pop(vm)))); break;
			case OP_DEFINE_GLOBAL: {
				LitString* name = READ_STRING();
				lit_table_set(vm, &vm->globals, name, lit_peek(vm, 0));
				lit_pop(vm);
				break;
			}
			case OP_GET_GLOBAL: {
				LitString* name = READ_STRING();
				LitValue value;

				if (!lit_table_get(&vm->globals, name, &value)) {
					runtime_error(vm, "Undefined variable '%s'", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}

				lit_push(vm, value);
				break;
			}
			case OP_SET_GLOBAL: {
				LitString* name = READ_STRING();

				if (lit_table_set(vm, &vm->globals, name, lit_peek(vm, 0))) {
					runtime_error(vm, "Undefined variable '%s'", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}

				break;
			}
			default: printf("Unhandled instruction %i\n!", *--vm->ip); UNREACHABLE();
		}
	}

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}