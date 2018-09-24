#include <stdio.h>
#include <zconf.h>
#include <string.h>
#include <time.h>

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

static bool last_native;

static bool call_value(LitVm* vm, LitValue callee, int arg_count) {
	last_native = false;

	if (IS_OBJECT(callee)) {
		switch (OBJECT_TYPE(callee)) {
			case OBJECT_CLOSURE: return call(vm, AS_CLOSURE(callee), arg_count);
			case OBJECT_NATIVE: {
				last_native = true;
				AS_NATIVE(callee)(vm);
				return true;
			}
			default: UNREACHABLE();
		}
	}

	runtime_error(vm, "Can only call functions and classes.");
	return false;
}

static void time_function(LitVm* vm) {
	lit_push(vm, MAKE_NUMBER_VALUE((double) clock() / CLOCKS_PER_SEC));
}

LitInterpretResult lit_execute(LitVm* vm, const char* code) {
	LitFunction *function = lit_compile(vm, code);

	define_native(vm, "time", time_function);

	if (function == NULL) {
		return INTERPRET_COMPILE_ERROR;
	}

	lit_push(vm, MAKE_OBJECT_VALUE(function));
	LitClosure* closure = lit_new_closure(vm, function);
	lit_pop(vm);

	call_value(vm, MAKE_OBJECT_VALUE(closure), 0);

	return lit_interpret(vm);
}

static void close_upvalues(LitVm* vm, LitValue* last) {
	while (vm->open_upvalues != NULL && vm->open_upvalues->value >= last) {
		LitUpvalue* upvalue = vm->open_upvalues;

		upvalue->closed = *upvalue->value;
		upvalue->value = &upvalue->closed;
		vm->open_upvalues = upvalue->next;
	}
}

static LitUpvalue* capture_upvalue(LitVm* vm, LitValue* local) {
	if (vm->open_upvalues == NULL) {
		vm->open_upvalues = lit_new_upvalue(vm, local);
		return vm->open_upvalues;
	}

	LitUpvalue* prev_upvalue = NULL;
	LitUpvalue* upvalue = vm->open_upvalues;

	while (upvalue != NULL && upvalue->value > local) {
		prev_upvalue = upvalue;
		upvalue = upvalue->next;
	}

	if (upvalue != NULL && upvalue->value == local) {
		return upvalue;
	}

	LitUpvalue* created_upvalue = lit_new_upvalue(vm, local);
	created_upvalue->next = upvalue;

	if (prev_upvalue == NULL) {
		vm->open_upvalues = created_upvalue;
	} else {
		prev_upvalue->next = created_upvalue;
	}

	return created_upvalue;
}

static void *functions[OP_CALL + 1];
static bool inited_functions;

LitInterpretResult lit_interpret(LitVm* vm) {
	if (!inited_functions) {
		inited_functions = true;

		functions[OP_RETURN] = &&op_return;
		functions[OP_CONSTANT] = &&op_constant;
		functions[OP_PRINT] = &&op_print;
		functions[OP_NEGATE] = &&op_negate;
		functions[OP_ADD] = &&op_add;
		functions[OP_SUBTRACT] = &&op_subtract;
		functions[OP_MULTIPLY] = &&op_multiply;
		functions[OP_DIVIDE] = &&op_divide;
		functions[OP_POP] = &&op_pop;
		functions[OP_NOT] = &&op_not;
		functions[OP_NIL] = &&op_nil;
		functions[OP_TRUE] = &&op_true;
		functions[OP_FALSE] = &&op_false;
		functions[OP_EQUAL] = &&op_equal;
		functions[OP_GREATER] = &&op_greater;
		functions[OP_LESS] = &&op_less;
		functions[OP_GREATER_EQUAL] = &&op_greater_equal;
		functions[OP_LESS_EQUAL] = &&op_less_equal;
		functions[OP_NOT_EQUAL] = &&op_not_equal;
		functions[OP_CLOSE_UPVALUE] = &&op_close_upvalue;
		functions[OP_DEFINE_GLOBAL] = &&op_define_global;
		functions[OP_GET_GLOBAL] = &&op_get_global;
		functions[OP_SET_GLOBAL] = &&op_set_global;
		functions[OP_GET_LOCAL] = &&op_get_local;
		functions[OP_SET_LOCAL] = &&op_set_local;
		functions[OP_GET_UPVALUE] = &&op_get_upvalue;
		functions[OP_SET_UPVALUE] = &&op_set_upvalue;
		functions[OP_JUMP] = &&op_jump;
		functions[OP_JUMP_IF_FALSE] = &&op_jump_if_false;
		functions[OP_LOOP] = &&op_loop;
		functions[OP_CLOSURE] = &&op_closure;
		functions[OP_CALL] = &&op_call;
	}

#ifdef DEBUG_TRACE_EXECUTION
	printf("== start vm ==\n");
#endif

	LitFrame* frame = &vm->frames[vm->frame_count - 1];
	u_int8_t* ip = frame->ip;
	LitValue* stack = vm->stack;

#define READ_BYTE() (*ip++)
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_SHORT() (ip += 2, (uint16_t) ((ip[-2] << 8) | ip[-1]))
#define PUSH(value) { *vm->stack_top = value; vm->stack_top++; }
#define POP() ({ assert(vm->stack_top > stack); vm->stack_top--; *vm->stack_top; })
#define PEEK(depth) (vm->stack_top[-1 - depth])

	while (true) {
#ifdef DEBUG_TRACE_EXECUTION
		if (vm->stack != vm->stack_top) {
			for (LitValue* slot = vm->stack; slot < vm->stack_top; slot++) {
				printf("[%s]", lit_to_string(vm, *slot));
			}

			printf("\n");
		}

		lit_disassemble_instruction(vm, &frame->closure->function->chunk, (int) (ip - frame->closure->function->chunk.code));
#endif

		goto *functions[*ip++];

		op_constant: {
			PUSH(READ_CONSTANT());
			continue;
		};

		op_return: {
			LitValue result = POP();
			close_upvalues(vm, frame->slots);

			vm->frame_count--;

			if (vm->frame_count == 0) {
				return INTERPRET_OK;
			}

			vm->stack_top = frame->slots;
			PUSH(result);
			frame = &vm->frames[vm->frame_count - 1];
			ip = frame->ip;
			break;
		};

		op_print: {
			printf("%s\n", lit_to_string(vm, POP()));
			continue;
		};

		op_negate: {
			if (!IS_NUMBER(vm->stack_top[-1])) {
				runtime_error(vm, "Operand must be a number!\n");
				return INTERPRET_RUNTIME_ERROR;
			}

			vm->stack_top[-1] = MAKE_NUMBER_VALUE(-AS_NUMBER(vm->stack_top[-1]));
			continue;
		};

		op_add: {
			LitValue b = POP();
			LitValue a = POP();

			if (IS_NUMBER(a) && IS_NUMBER(b)) {
				PUSH(MAKE_NUMBER_VALUE(AS_NUMBER(a) + AS_NUMBER(b)));
			} else {
				char *as = lit_to_string(vm, a);
				char *bs = lit_to_string(vm, b);

				size_t al = strlen(as);
				size_t bl = strlen(bs);
				size_t length = al + bl;

				char* chars = ALLOCATE(vm, char, length + 1);

				memcpy(chars, as, al);
				memcpy(chars + al, bs, bl);
				chars[length] = '\0';

				PUSH(MAKE_OBJECT_VALUE(lit_make_string(vm, chars, length)));
			}
			continue;
		};

		op_subtract: {
			LitValue b = POP();
			LitValue a = POP();

			if (IS_NUMBER(a) && IS_NUMBER(b)) {
				PUSH(MAKE_NUMBER_VALUE(AS_NUMBER(a) - AS_NUMBER(b)));
			} else {
				runtime_error(vm, "Operands must be two numbers");
				return INTERPRET_RUNTIME_ERROR;
			}
			continue;
		};

		op_multiply: {
			LitValue b = POP();
			LitValue a = POP();

			if (IS_NUMBER(a) && IS_NUMBER(b)) {
				PUSH(MAKE_NUMBER_VALUE(AS_NUMBER(a) * AS_NUMBER(b)));
			} else {
				runtime_error(vm, "Operands must be two numbers");
				return INTERPRET_RUNTIME_ERROR;
			}
			continue;
		};

		op_divide: {
			LitValue b = POP();
			LitValue a = POP();

			if (IS_NUMBER(a) && IS_NUMBER(b)) {
				PUSH(MAKE_NUMBER_VALUE(AS_NUMBER(a) / AS_NUMBER(b)));
			} else {
				runtime_error(vm, "Operands must be two numbers");
				return INTERPRET_RUNTIME_ERROR;
			}
			continue;
		};

		op_not: {
			vm->stack_top[-1] = MAKE_BOOL_VALUE(lit_is_false(vm->stack_top[-1]));
			continue;
		};

		op_nil: {
			PUSH(NIL_VALUE);
			continue;
		};

		op_true: {
			PUSH(TRUE_VALUE);
			continue;
		};

		op_false: {
			PUSH(FALSE_VALUE);
			continue;
		};

		op_equal: {
			vm->stack_top[-1] = MAKE_BOOL_VALUE(lit_are_values_equal(POP(), vm->stack_top[-2]));
			continue;
		};

		op_greater: {
			LitValue a = POP();

			if (IS_NUMBER(a) && IS_NUMBER(vm->stack_top[-1])) {
				vm->stack_top[-1] = MAKE_BOOL_VALUE(AS_NUMBER(a) <= AS_NUMBER(vm->stack_top[-1]));
			} else {
				runtime_error(vm, "Operands must be two numbers");
				return INTERPRET_RUNTIME_ERROR;
			}
			continue;
		};

		op_less: {
			LitValue a = POP();

			if (IS_NUMBER(a) && IS_NUMBER(vm->stack_top[-1])) {
				vm->stack_top[-1] = MAKE_BOOL_VALUE(AS_NUMBER(a) >= AS_NUMBER(vm->stack_top[-1]));
			} else {
				runtime_error(vm, "Operands must be two numbers");
				return INTERPRET_RUNTIME_ERROR;
			}

			continue;
		};

		op_greater_equal: {
			LitValue a = POP();

			if (IS_NUMBER(a) && IS_NUMBER(vm->stack_top[-1])) {
				vm->stack_top[-1] = MAKE_BOOL_VALUE(AS_NUMBER(a) < AS_NUMBER(vm->stack_top[-1]));
			} else {
				runtime_error(vm, "Operands must be two numbers");
				return INTERPRET_RUNTIME_ERROR;
			}

			continue;
		};

		op_less_equal: {
			LitValue a = POP();

			if (IS_NUMBER(a) && IS_NUMBER(vm->stack_top[-1])) {
				vm->stack_top[-1] = MAKE_BOOL_VALUE(AS_NUMBER(a) > AS_NUMBER(vm->stack_top[-1]));
			} else {
				runtime_error(vm, "Operands must be two numbers");
				return INTERPRET_RUNTIME_ERROR;
			}

			continue;
		};

		op_not_equal: {
			vm->stack_top[-2] = MAKE_BOOL_VALUE(!lit_are_values_equal(POP(), vm->stack_top[-2]));
			continue;
		};

		op_pop: {
			POP();
			continue;
		};

		op_close_upvalue: {
			close_upvalues(vm, vm->stack_top - 1);
			continue;
		};

		op_define_global: {
			lit_table_set(vm, &vm->globals, READ_STRING(), vm->stack_top[-1]);
			POP();
			continue;
		};

		op_get_global: {
			LitString* name = READ_STRING();
			LitValue value;

			if (!lit_table_get(&vm->globals, name, &value)) {
				runtime_error(vm, "Undefined variable '%s'", name->chars);
				return INTERPRET_RUNTIME_ERROR;
			}

			PUSH(value);
			continue;
		};

		op_set_global: {
			LitString* name = READ_STRING();

			if (lit_table_set(vm, &vm->globals, name, POP())) {
				runtime_error(vm, "Undefined variable '%s'", name->chars);
				return INTERPRET_RUNTIME_ERROR;
			}

			continue;
		};

		op_get_local: {
			PUSH(frame->slots[READ_BYTE()]);
			continue;
		};

		op_set_local: {
			frame->slots[READ_BYTE()] = vm->stack_top[-1];
			continue;
		};

		op_get_upvalue: {
			PUSH(*frame->closure->upvalues[READ_BYTE()]->value);
			continue;
		};

		op_set_upvalue: {
			*frame->closure->upvalues[READ_BYTE()]->value = vm->stack_top[-1];
			continue;
		};

		op_jump: {
			ip += READ_SHORT();
			continue;
		};

		op_jump_if_false: {
			uint16_t offset = READ_SHORT();

			if (lit_is_false(PEEK(0))) {
				ip += offset;
			}

			continue;
		};

		op_loop: {
			ip -= READ_SHORT();
			continue;
		};

		op_closure: {
			LitFunction* function = AS_FUNCTION(READ_CONSTANT());

			LitClosure* closure = lit_new_closure(vm, function);
			PUSH(MAKE_OBJECT_VALUE(closure));

			for (int i = 0; i < closure->upvalue_count; i++) {
				uint8_t is_local = READ_BYTE();
				uint8_t index = READ_BYTE();

				if (is_local) {
					closure->upvalues[i] = capture_upvalue(vm, frame->slots + index);
				} else {
					closure->upvalues[i] = frame->closure->upvalues[index];
				}
			}

			continue;
		};

		op_call: {
			int arg_count = AS_NUMBER(POP());

			if (!call_value(vm, PEEK(arg_count), arg_count)) {
				return INTERPRET_RUNTIME_ERROR;
			}

			if (!last_native) {
				frame = &vm->frames[vm->frame_count - 1];
				ip = frame->ip;
			}

			continue;
		};
	}

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef READ_SHORT
#undef PUSH
#undef POP
#undef PEEK

	return INTERPRET_OK;
}