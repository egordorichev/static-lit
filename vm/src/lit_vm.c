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
	vm->open_upvalues = NULL;
	vm->frame_count = 0;
}

static void define_native(LitVm* vm, const char* name, LitNativeFn function) {
	lit_push(vm, MAKE_OBJECT_VALUE(lit_copy_string(vm, name, (int) strlen(name))));
	lit_push(vm, MAKE_OBJECT_VALUE(lit_new_native(vm, function)));
	lit_table_set(vm, &vm->globals, AS_STRING(vm->stack[0]), vm->stack[1]);
	lit_pop(vm);
	lit_pop(vm);
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

	for (int i = vm->frame_count - 1; i >= 0; i--) {
		LitFrame* frame = &vm->frames[i];
		LitFunction* function = frame->closure->function;

		size_t instruction = frame->ip - function->chunk.code - 1;
		fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);

		if (function->name == NULL) {
			fprintf(stderr, "script\n");
		} else {
			fprintf(stderr, "%s()\n", function->name->chars);
		}
	}

	reset_stack(vm);
}

static bool call(LitVm* vm, LitClosure* closure, int arg_count) {
	if (arg_count != closure->function->arity) {
		runtime_error(vm, "Expected %d arguments but got %d",	closure->function->arity, arg_count);
		return false;
	}

	if (vm->frame_count == FRAMES_MAX) {
		runtime_error(vm, "Stack overflow");
		return false;
	}

	LitFrame* frame = &vm->frames[vm->frame_count++];

	frame->closure = closure;
	frame->ip = closure->function->chunk.code;
	frame->slots = vm->stack_top - (arg_count + 1);

	return true;
}

static bool call_value(LitVm* vm, LitValue callee, int arg_count) {
	if (IS_OBJECT(callee)) {
		switch (OBJECT_TYPE(callee)) {
			case OBJECT_CLOSURE: return call(vm, AS_CLOSURE(callee), arg_count);
			case OBJECT_NATIVE: {
				/*LitNativeFn native = AS_NATIVE(callee);
				Value result = native(argCount, vm.stackTop - argCount);
				vm.stackTop -= argCount + 1;
				push(result);*/

				return true;
			}
		}
	}

	runtime_error(vm, "Can only call functions and classes.");
	return false;
}

LitInterpretResult lit_execute(LitVm* vm, const char* code) {
	LitFunction *function = lit_compile(vm, code);

	if (function == NULL) {
		return INTERPRET_COMPILE_ERROR;
	}

	lit_push(vm, MAKE_OBJECT_VALUE(function));
	LitClosure* closure = lit_new_closure(vm, function);
	lit_pop(vm);

	call_value(vm, MAKE_OBJECT_VALUE(closure), 0);

	return lit_interpret(vm);
}

LitInterpretResult lit_interpret(LitVm* vm) {
	// TODO: store ip in a local var
	LitFrame* frame = &vm->frames[vm->frame_count - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
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

		lit_disassemble_instruction(&frame->closure->function->chunk, (int) (frame->ip - frame->closure->function->chunk.code));
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
			case OP_SET_UPVALUE: {

			}
			case OP_GET_UPVALUE: {

			}
			case OP_GET_LOCAL: {
				uint8_t slot = READ_BYTE();
				lit_push(vm, frame->slots[slot]);
				break;
			}
			case OP_SET_LOCAL: {
				uint8_t slot = READ_BYTE();
				frame->slots[slot] = lit_peek(vm, 0);
				break;
			}
			default: printf("Unhandled instruction %i\n!", *--frame->ip); UNREACHABLE();
		}
	}

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}