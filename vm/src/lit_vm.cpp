#include <cstdarg>
#include "lit_vm.hpp"
#include "lit_debug.hpp"

LitVm::LitVm() {
  reset_stack();
}

LitVm::~LitVm() {

}

static bool is_false(LitValue value) {
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value)) || (IS_NUMBER(value) && AS_NUMBER(value) == 0);
}

InterpretResult LitVm::run_chunk(LitChunk* cnk) {
  chunk = cnk;
  ip = cnk->get_code();

#define READ_BYTE() (*ip++)
#define READ_CONSTANT() (chunk->get_constants()->get(READ_BYTE()))
#define BINARY_OP(type, op) \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      runtime_error("Operands must be numbers."); \
      return INTERPRET_RUNTIME_ERROR; \
    } \
    do { \
      double b = AS_NUMBER(pop()); \
      double a = AS_NUMBER(pop()); \
      push(type(a op b)); \
    } while (false)

  uint8_t instruction;

  for (;;) {
    instruction = READ_BYTE();

#ifdef DEBUG_TRACE_EXECUTION
    if (stack != stack_top) {
      printf("     | ");

      for (LitValue* slot = stack; slot < stack_top; slot++) {
        printf("[ ");
        lit_print_value(*slot);
        printf(" ]");
      }

      printf("\n");
    }

    lit_disassemble_instruction(chunk, (int) (ip - chunk->get_code() - 1));
#endif

		switch (instruction) {
			case OP_RETURN: {
				printf("Return: ");
				lit_print_value(pop());
				printf("\n");
				return INTERPRET_OK;
			}
			case OP_CONSTANT: {
				push(READ_CONSTANT());
				break;
			}
			case OP_NEGATE: {
				if (!IS_NUMBER(peek(0))) {
					runtime_error("Operand must be a number.");
					return INTERPRET_RUNTIME_ERROR;
				}

				push(MAKE_NUMBER_VALUE(-AS_NUMBER(pop())));
				break;
			}
			case OP_ADD: BINARY_OP(MAKE_NUMBER_VALUE, +); break;
			case OP_SUBTRACT: BINARY_OP(MAKE_NUMBER_VALUE, -); break;
			case OP_MULTIPLY: BINARY_OP(MAKE_NUMBER_VALUE, *); break;
			case OP_DIVIDE: BINARY_OP(MAKE_NUMBER_VALUE, /); break;
			case OP_NIL: push(NIL_VAL); break;
			case OP_TRUE: push(MAKE_BOOL_VALUE(true)); break;
			case OP_FALSE: push(MAKE_BOOL_VALUE(false)); break;
			case OP_NOT: push(MAKE_BOOL_VALUE(is_false(pop()))); break;
			case OP_EQUAL: push(MAKE_BOOL_VALUE(lit_values_are_equal(pop(), pop())));	break;
			case OP_GREATER: BINARY_OP(MAKE_BOOL_VALUE, >); break;
			case OP_LESS: BINARY_OP(MAKE_BOOL_VALUE, <); break;
		}
	}

#undef BINARY_OP
#undef READ_CONSTANT
#undef READ_BYTE
}

void LitVm::reset_stack() {
  stack_top = stack;
}

void LitVm::push(LitValue value) {
  *stack_top = value;
  stack_top++;
}

LitValue LitVm::pop() {
  stack_top--;
  return *stack_top;
}

LitValue LitVm::peek(int depth) {
	return stack_top[-1 - depth];
}

void LitVm::runtime_error(const char* format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	size_t instruction = ip - chunk->get_code();
	fprintf(stderr, "[line %d] in script\n", chunk->get_line(instruction));

	reset_stack();
}