#include <cstdarg>
#include <string>
#include <cstring>

#include "lit_vm.hpp"
#include "lit_debug.hpp"
#include "lit_object.hpp"
#include "lit_value.hpp"

static LitVm* active;

LitVm::LitVm() {
  reset_stack();
  active = this;

  objects = nullptr;
  bytes_allocated = 0;
  next_gc = 1024 * 1024;
  gray_count = 0;
  gray_capacity = 0;
  gray_stack = nullptr;
}

LitVm::~LitVm() {
  active = this;
  free_objects();
}

static bool is_false(LitValue value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value))
    || (IS_NUMBER(value) && AS_NUMBER(value) == 0);
}

LitVm* lit_get_active_vm() {
  return active;
}

InterpretResult LitVm::run_chunk(LitChunk* cnk) {
  active = this;
  chunk = cnk;
  ip = cnk->get_code();

#define READ_BYTE() (*ip++)
#define READ_CONSTANT() (chunk->get_constants()->get(READ_BYTE()))
#define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))
#define READ_STRING() AS_STRING(READ_CONSTANT())
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
				//printf("Return: ");
				//lit_print_value(pop());
				//printf("\n");
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
			case OP_ADD: {
				LitValue bv = pop();
				LitValue av = pop();

				if (IS_NUMBER(av) && IS_NUMBER(bv)) {
					push(MAKE_NUMBER_VALUE(AS_NUMBER(av) + AS_NUMBER(bv)));
				} else if ((IS_STRING(av) || IS_NUMBER(av)) || (IS_STRING(bv) || IS_NUMBER(bv))) {
					const char* a = lit_to_string(av);
					const char* b = lit_to_string(bv);

					int al = strlen(a);
					int bl = strlen(b);
					int len = al + bl;

					char* chars = ALLOCATE(char, len + 1);

					memcpy(chars, a, al);
					memcpy(chars + al, b, bl);
					chars[len] = '\0';

					push(MAKE_OBJECT_VALUE(lit_take_string(chars, len)));
				} else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
					push(MAKE_NUMBER_VALUE(AS_NUMBER(pop()) + AS_NUMBER(pop())));
				} else {
					runtime_error("Operands must be two numbers or two strings.");
					return INTERPRET_RUNTIME_ERROR;
				}

				break;
			}
			case OP_PRINT: {
				printf("%s\n", lit_to_string(pop()));
				break;
			}
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
      case OP_AND: {
        LitValue right = pop();
        LitValue left = pop();

        if (IS_BOOL(left) && IS_BOOL(right)) {
          push(MAKE_BOOL_VALUE(AS_BOOL(left) && AS_BOOL(right)));
        } else {
          runtime_error("Operands must be two booleans.");
        }

        break;
      }
      case OP_OR: {
        LitValue right = pop();
        LitValue left = pop();

        if (IS_BOOL(left) && IS_BOOL(right)) {
          push(MAKE_BOOL_VALUE(AS_BOOL(left) || AS_BOOL(right)));
        } else {
          runtime_error("Operands must be two booleans.");
        }

        break;
      }
			case OP_POP: pop(); break;
			case OP_JUMP_IF_FALSE: {
				uint16_t offset = READ_SHORT();

				if (is_false(peek(0))) {
					ip += offset;
				}

				break;
			}
			case OP_JUMP: ip += READ_SHORT(); break;
			case OP_DEFINE_GLOBAL: {
				LitString* name = READ_STRING();
				globals.set(name, peek(0));
				pop();
				break;
			}
		}
	}

#undef BINARY_OP
#undef READ_CONSTANT
#undef READ_BYTE
#undef READ_SHORT
#undef READ_STRING
}

void LitVm::reset_stack() {
  stack_top = stack;
}

void LitVm::push(LitValue value) {
  *stack_top = value;
  stack_top++;
}

LitValue LitVm::pop() {
  if (stack_top == stack) {
    runtime_error("Trying to pop below 0.");
  }

  stack_top--;
  return *stack_top;
}

LitValue LitVm::peek(int depth) {
  return stack_top[-1 - depth];
}

void LitVm::runtime_error(const char* format, ...) {
  va_list args;

  va_start(args, format);
#ifndef DEBUG
  vfprintf(stderr, format, args);
#else
  vprintf(format, args);
#endif
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = ip - chunk->get_code();

#ifndef DEBUG
  fprintf(stderr, "[line %d] in script\n", chunk->get_line(instruction));
#else
  printf("[line %d] in script\n", chunk->get_line(instruction));
#endif

  reset_stack();
}