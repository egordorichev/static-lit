#include <stdio.h>
#include <zconf.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

#include <vm/lit_vm.h>
#include <compiler/lit_parser.h>
#include <std/lit_std.h>
#include <lit_debug.h>
#include <vm/lit_object.h>

static void runtime_error(LitVm* vm, const char* format, ...) {
	va_list args;
	va_start(args, format);
	fprintf(stderr, "Runtime error: ");
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);

	for (int i = vm->frame_count - 1; i >= 0; i--) {
		LitFrame* frame = &vm->frames[i];
		LitFunction* function = frame->closure->function;
		fprintf(stderr, "\tat %s():%ld\n", function->name->chars, lit_chunk_get_line(&function->chunk, frame->ip - function->chunk.code - 1));
	}

	vm->abort = true;
	// Causes issues with error() function
	// reset_stack(vm);
}

static bool call(LitVm* vm, LitClosure* closure, int arg_count) {
	if (vm->frame_count == FRAMES_MAX) {
		runtime_error(vm, "Stack overflow");
		return false;
	}

	LitFrame* frame = &vm->frames[vm->frame_count++];

	frame->closure = closure;
	frame->ip = closure->function->chunk.code;

	if (DEBUG_TRACE_EXECUTION) {
		printf("== %s ==\n", frame->closure->function->name == NULL ? "top-level" : frame->closure->function->name->chars);
	}

	return true;
}

static bool call_value(LitVm* vm, LitValue callee, int arg_count, bool static_init) {
	if (IS_OBJECT(callee)) {
		switch (OBJECT_TYPE(callee)) {
			case OBJECT_CLOSURE: {
				return call(vm, AS_CLOSURE(callee), arg_count);
			}
			default: UNREACHABLE();
		}
	}

	runtime_error(vm, "Can only call functions and classes");
	return false;
}

static bool interpret(LitVm* vm) {
	static void* dispatch_table[] = {
#define OPCODE(name) &&CODE_##name,
#include <vm/lit_opcode.h>
#undef OPCODE
	};

	vm->abort = false;

	register LitFrame* frame = &vm->frames[vm->frame_count - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() (frame->closure->function->chunk.constants.values[READ_SHORT()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_SHORT() (frame->ip += 2, (uint16_t) ((frame->ip[-2] << 8) | frame->ip[-1]))
#define CASE_CODE(name) CODE_##name:

	while (true) {
		if (vm->abort) {
			return false;
		}

		if (DEBUG_TRACE_EXECUTION) {
			lit_disassemble_instruction(MM(vm), &frame->closure->function->chunk, (uint64_t) (frame->ip - frame->closure->function->chunk.code));
		}

		goto *dispatch_table[*frame->ip++];

		CASE_CODE(CONSTANT) {
			vm->registers[READ_BYTE()] = READ_CONSTANT();
			continue;
		};

		CASE_CODE(CONSTANT_LONG) {
			vm->registers[READ_BYTE()] = READ_CONSTANT_LONG();
			continue;
		};

		CASE_CODE(ADD) {
			vm->registers[READ_BYTE()] = MAKE_NUMBER_VALUE(AS_NUMBER(vm->registers[READ_BYTE()]) + AS_NUMBER(vm->registers[READ_BYTE()]));
			continue;
		};

		CASE_CODE(EXIT) {
			vm->frame_count--;

			if (vm->frame_count == 0) {
				return false;
			}

			frame = &vm->frames[vm->frame_count - 1];

			if (DEBUG_TRACE_EXECUTION) {
				printf("== %s ==\n", frame->closure->function->name == NULL ? "top-level" : frame->closure->function->name->chars);
			}

			continue;
		};

		CASE_CODE(RETURN) {
			vm->frame_count--;

			if (vm->frame_count == 0) {
				return false;
			}

			frame = &vm->frames[vm->frame_count - 1];

			if (DEBUG_TRACE_EXECUTION) {
				printf("== %s ==\n", frame->closure->function->name == NULL ? "top-level" : frame->closure->function->name->chars);
			}
		};

		/*CASE_CODE(STATIC_INIT) {
			if (!call_value(vm, PEEK(0), 0, true)) {
				return false;
			}

			frame = &vm->frames[vm->frame_count - 1];
			continue;
		};

		CASE_CODE(NEGATE) {
			vm->stack_top[-1] = MAKE_NUMBER_VALUE(-AS_NUMBER(vm->stack_top[-1]));
			continue;
		};

		CASE_CODE(SUBTRACT) {
			LitValue b = POP();
			PUSH(MAKE_NUMBER_VALUE(AS_NUMBER(POP()) - AS_NUMBER(b)));

			continue;
		};

		CASE_CODE(MULTIPLY) {
			LitValue b = POP();
			PUSH(MAKE_NUMBER_VALUE(AS_NUMBER(POP()) * AS_NUMBER(b)));

			continue;
		};

		CASE_CODE(DIVIDE) {
			LitValue b = POP();
			PUSH(MAKE_NUMBER_VALUE(AS_NUMBER(POP()) / AS_NUMBER(b)));

			continue;
		};

		CASE_CODE(MODULO) {
			PUSH(MAKE_NUMBER_VALUE(fmod(AS_NUMBER(POP()), AS_NUMBER(POP()))));
			continue;
		};

		CASE_CODE(FLOOR) {
			vm->stack_top[-1] = MAKE_NUMBER_VALUE(floor(AS_NUMBER(vm->stack_top[-1])));
			continue;
		};

		CASE_CODE(POWER) {
			PUSH(MAKE_NUMBER_VALUE(pow(AS_NUMBER(POP()), AS_NUMBER(POP()))));
			continue;
		};

		CASE_CODE(ROOT) {
			PUSH(MAKE_NUMBER_VALUE(pow(AS_NUMBER(POP()), 1.0 / AS_NUMBER(POP()))));
			continue;
		};

		CASE_CODE(SQUARE) {
			PUSH(MAKE_NUMBER_VALUE(sqrt(AS_NUMBER(POP()))));
			continue;
		};

		CASE_CODE(NOT) {
			vm->stack_top[-1] = MAKE_BOOL_VALUE(lit_is_false(vm->stack_top[-1]));
			continue;
		};

		CASE_CODE(NIL) {
			PUSH(NIL_VALUE);
			continue;
		};

		CASE_CODE(TRUE) {
			PUSH(TRUE_VALUE);
			continue;
		};

		CASE_CODE(FALSE) {
			PUSH(FALSE_VALUE);
			continue;
		};

		CASE_CODE(EQUAL) {
			LitValue a = POP();
			vm->stack_top[-1] = MAKE_BOOL_VALUE(AS_NUMBER(a) == AS_NUMBER(vm->stack_top[-1]));

			continue;
		};

		CASE_CODE(GREATER) {
			LitValue a = POP();
			vm->stack_top[-1] = MAKE_BOOL_VALUE(AS_NUMBER(a) < AS_NUMBER(vm->stack_top[-1]));

			continue;
		};

		CASE_CODE(LESS) {
			LitValue a = POP();
			vm->stack_top[-1] = MAKE_BOOL_VALUE(AS_NUMBER(a) > AS_NUMBER(vm->stack_top[-1]));

			continue;
		};

		CASE_CODE(GREATER_EQUAL) {
			LitValue a = POP();
			vm->stack_top[-1] = MAKE_BOOL_VALUE(AS_NUMBER(a) <= AS_NUMBER(vm->stack_top[-1]));

			continue;
		};

		CASE_CODE(LESS_EQUAL) {
			LitValue a = POP();
			vm->stack_top[-1] = MAKE_BOOL_VALUE(AS_NUMBER(a) >= AS_NUMBER(vm->stack_top[-1]));

			continue;
		};

		CASE_CODE(NOT_EQUAL) {
			LitValue a = POP();
			vm->stack_top[-1] = MAKE_BOOL_VALUE(AS_NUMBER(a) != AS_NUMBER(vm->stack_top[-1]));

			continue;
		};

		CASE_CODE(POP) {
			POP();
			continue;
		};

		CASE_CODE(CLOSE_UPVALUE) {
			close_upvalues(vm, vm->stack_top - 1);
			continue;
		};

		CASE_CODE(DEFINE_GLOBAL) {
			lit_table_set(MM(vm), &vm->globals, READ_STRING(), vm->stack_top[-1]);
			POP();

			continue;
		};

		CASE_CODE(GET_GLOBAL) {
			LitString* name = READ_STRING();
			LitValue *value = lit_table_get(&vm->globals, name);
			PUSH(*value);

			continue;
		};

		CASE_CODE(SET_GLOBAL) {
			lit_table_set(MM(vm), &vm->globals, READ_STRING(), PEEK(0));
			continue;
		};

		CASE_CODE(GET_LOCAL) {
			PUSH(frame->slots[READ_BYTE()]);

			continue;
		};

		CASE_CODE(SET_LOCAL) {
			frame->slots[READ_BYTE()] = vm->stack_top[-1];
			continue;
		};

		CASE_CODE(GET_UPVALUE) {
			PUSH(*frame->closure->upvalues[READ_BYTE()]->value);
			continue;
		};

		CASE_CODE(SET_UPVALUE) {
			*frame->closure->upvalues[READ_BYTE()]->value = vm->stack_top[-1];
			continue;
		};

		CASE_CODE(JUMP) {
			frame->ip += READ_SHORT();
			continue;
		};

		CASE_CODE(JUMP_IF_FALSE) {
			uint16_t offset = READ_SHORT();

			if (lit_is_false(PEEK(0))) {
				frame->ip += offset;
			}

			continue;
		};

		CASE_CODE(LOOP) {
			frame->ip -= READ_SHORT();
			continue;
		};

		CASE_CODE(CLOSURE) {
			LitFunction* function = AS_FUNCTION(READ_CONSTANT());

			LitClosure* closure = lit_new_closure(MM(vm), function);
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

		CASE_CODE(CALL) {
			int arg_count = READ_BYTE();

			if (!call_value(vm, PEEK(arg_count), arg_count, false)) {
				return false;
			}

			if (!last_native) {
				frame = &vm->frames[vm->frame_count - 1];
			}

			continue;
		};

		CASE_CODE(CLASS) {
			create_class(vm, READ_STRING(), NULL);
			continue;
		};

		CASE_CODE(SUBCLASS) {
			LitValue super = POP();

			if (!IS_CLASS(super)) {
				runtime_error(vm, "Superclass must be a class");
				return false;
			}

			create_class(vm, READ_STRING(), AS_CLASS(super));
			continue;
		};

		CASE_CODE(METHOD) {
			define_method(vm, READ_STRING());
			continue;
		};

		CASE_CODE(GET_FIELD) {
			LitValue from = PEEK(0);

			if (IS_CLASS(from)) {
				LitString *name = READ_STRING();
				LitClass *class = AS_CLASS(from);
				LitValue *field = lit_table_get(&class->static_fields, name);

				if (field != NULL) {
					POP();
					PUSH(*field);
				} else {
					LitValue *method = lit_table_get(&class->static_methods, name);

					if (method != NULL) {
						POP();
						PUSH(*method);
					} else {
						method = lit_table_get(&vm->class_class->methods, name);

						if (method != NULL) {
							POP();
							PUSH(*method);
						} else {
							runtime_error(vm, "Class %s has no static field or method %s", class->name->chars, name->chars);
						}
					}
				}
			} else {
				LitClass *type = NULL;

				if (IS_NIL(from)) {
					runtime_error(vm, "Attempt to get a field from a nil value");
					return false;
				} else if (IS_STRING(from)) {
					type = vm->string_class;
				} else if (IS_CLASS(from)) {
					type = vm->class_class;
				} else if (IS_NUMBER(from)) {
					double number = AS_NUMBER(from);
					double temp;

					if (modf(number, &temp) == 0) {
						type = vm->int_class;
					} else {
						type = vm->double_class;
					}
				}

				if (type != NULL || IS_INSTANCE(from)) {
					LitInstance *instance = AS_INSTANCE(from);
					LitString *name = READ_STRING();
					LitValue *method = lit_table_get(type == NULL ? &instance->type->methods : &type->methods, name);

					if (method != NULL) {
						POP();
						PUSH(*method);
					} else {
						LitValue *field = lit_table_get(&instance->fields, name);

						if (field != NULL) {
							POP();
							PUSH(*field);
						} else {
							runtime_error(vm, "Class %s has no field or method %s", instance->type->name->chars, name->chars);
						}
					}
				} else {
					runtime_error(vm, "Only instances and classes have properties");
					return false;
				}
			}

			continue;
		};

		CASE_CODE(SET_FIELD) {
			LitValue from = PEEK(1);

			if (IS_CLASS(from)) {
				LitValue value = POP();
				lit_table_set(MM(vm), &AS_CLASS(from)->static_fields, READ_STRING(), value);

				POP();
				PUSH(value);
			} else if (IS_INSTANCE(from)) {
				LitInstance* instance = AS_INSTANCE(PEEK(1));
				LitValue value = POP();

				lit_table_set(MM(vm), &instance->fields, READ_STRING(), value);

				POP();
				PUSH(value);
			} else {
				printf("%s\n", "test");
				runtime_error(vm, "Only instances and classes have fields");
				return false;
			}

			continue;
		};

		CASE_CODE(INVOKE) {
			int arg_count = READ_BYTE();

			if (!invoke(vm, arg_count)) {
				return false;
			}

			frame = &vm->frames[vm->frame_count - 1];
			continue;
		};

		CASE_CODE(DEFINE_FIELD) {
			if (!IS_CLASS(PEEK(1))) {
				runtime_error(vm, "Can't define a field in non-class");
				return false;
			}

			LitClass* class = AS_CLASS(PEEK(1));
			lit_table_set(MM(vm), &class->fields, READ_STRING(), POP());

			continue;
		};

		CASE_CODE(DEFINE_METHOD) {
			LitString* name = READ_STRING();
			LitValue method = POP();
			LitClass* class = AS_CLASS(PEEK(0));

			lit_table_set(MM(vm), &class->methods, name, method);
			continue;
		};

		CASE_CODE(SUPER) {
			LitString* name = READ_STRING();
			LitInstance* instance = AS_INSTANCE(PEEK(0));
			LitValue *method = lit_table_get(&instance->type->super->methods, name);

			if (method == NULL) {
				runtime_error(vm, "Undefined method %s", name->chars);
				return false;
			}

			last_super = true;

			LitMethod* bound = lit_new_bound_method(MM(vm), MAKE_OBJECT_VALUE(instance), AS_CLOSURE(*method));
			PUSH(MAKE_OBJECT_VALUE(bound));

			continue;
		};

		CASE_CODE(DEFINE_STATIC_FIELD) {
			if (!IS_CLASS(PEEK(1))) {
				runtime_error(vm, "Can't define a field in non-class");
				return false;
			}

			LitClass* class = AS_CLASS(PEEK(1));
			lit_table_set(MM(vm), &class->static_fields, READ_STRING(), POP());

			continue;
		};

		CASE_CODE(DEFINE_STATIC_METHOD) {
			LitString* name = READ_STRING();
			LitValue method = POP();
			LitClass* class = AS_CLASS(PEEK(0));

			lit_table_set(MM(vm), &class->static_methods, name, method);
			continue;
		};

		CASE_CODE(IS) {
			LitClass* class = AS_CLASS(POP());
			LitInstance* instance = AS_INSTANCE(POP());
			LitClass* type = instance->type;
			bool found = false;

			while (type != NULL) {
				if (type == class) {
					PUSH(TRUE_VALUE);
					found = true;
					break;
				}

				type = type->super;
			}

			if (!found) {
				PUSH(FALSE_VALUE);
			}

			continue;
		};*/

		runtime_error(vm, "Unknown opcode!");
	}

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef READ_SHORT
#undef PUSH
#undef POP
#undef PEEK
#undef CASE_CODE

	return true;
}

void lit_init_vm(LitVm* vm) {
	LitMemManager* manager = (LitMemManager*) vm;

	manager->bytes_allocated = 0;
	manager->type = MANAGER_VM;
	manager->objects = NULL;

	lit_init_table(&manager->strings);
	lit_init_table(&vm->globals);

	vm->open_upvalues = NULL;
	vm->frame_count = 0;

	vm->next_gc = 1024 * 1024;
	vm->gray_capacity = 0;
	vm->gray_count = 0;
	vm->gray_stack = NULL;

	vm->class_class = NULL;
	vm->object_class = NULL;
	vm->string_class = NULL;
	vm->int_class = NULL;
	vm->double_class = NULL;
}

void lit_free_vm(LitVm* vm) {
	LitMemManager* manager = (LitMemManager*) vm;

	if (DEBUG_TRACE_MEMORY_LEAKS) {
		printf("Bytes allocated before freeing vm: %ld\n", ((LitMemManager*) vm)->bytes_allocated);
	}

	lit_free_table(MM(vm), &manager->strings);
	lit_free_table(MM(vm), &vm->globals);
	lit_free_objects(MM(vm));

	vm->init_string = NULL;

	if (DEBUG_TRACE_MEMORY_LEAKS) {
		printf("Bytes allocated after freeing vm: %ld\n", ((LitMemManager*) vm)->bytes_allocated);
	}
}

bool lit_execute(LitVm* vm, LitFunction* function) {
	if (!DEBUG_NO_EXECUTE) {
		call_value(vm, MAKE_OBJECT_VALUE(lit_new_closure(MM(vm), function)), 0, false);
		return interpret(vm);
	}

	return true;
}

bool lit_eval(const char* source_code) {
	LitCompiler compiler;
	lit_init_compiler(&compiler);
	LitLibRegistry* std = lit_create_std(&compiler);

	LitFunction* function = lit_compile(&compiler, source_code);
	lit_free_compiler(&compiler);

	if (function == NULL) {
		return false;
	}

	LitVm vm;
	lit_init_vm(&vm);

	lit_table_add_all(MM(&vm), &vm.mem_manager.strings, &compiler.mem_manager.strings);
	vm.init_string = lit_copy_string(MM(&vm), "init", 4);

	lit_define_lib(&vm, std);

	bool had_error = lit_execute(&vm, function);

	lit_free_vm(&vm);
	lit_free_bytecode_objects(&compiler);

	return !had_error;
}

void lit_vm_define_native(LitVm* vm, LitNativeRegistry* native) {
	LitString* str = lit_copy_string(MM(vm), native->name, (int) strlen(native->name));
	lit_table_set(MM(vm), &vm->globals, AS_STRING(MAKE_OBJECT_VALUE(str)), MAKE_OBJECT_VALUE(lit_new_native(MM(vm), native->function)));
}

void lit_vm_define_natives(LitVm* vm, LitNativeRegistry* natives) {
	int i = 0;
	LitNativeRegistry native;

	do {
		native = natives[i++];

		if (native.name != NULL) {
			lit_vm_define_native(vm, &native);
		}
	} while (native.name != NULL);
}

LitClass* lit_vm_define_class(LitVm* vm, LitType* type, LitClass* super) {
	LitClass* class = lit_new_class(MM(vm), type->name, super);
	lit_table_set(MM(vm), &vm->globals, type->name, MAKE_OBJECT_VALUE(class));

	if (vm->string_class == NULL && strcmp(type->name->chars, "String") == 0) {
		vm->string_class = class;
	} else if (vm->class_class == NULL && strcmp(type->name->chars, "Class") == 0) {
		vm->class_class = class;
	} else if (vm->object_class == NULL && strcmp(type->name->chars, "Object") == 0) {
		vm->object_class = class;
	} else if (vm->int_class == NULL && strcmp(type->name->chars, "Int") == 0) {
		vm->int_class = class;
	} else if (vm->double_class == NULL && strcmp(type->name->chars, "Double") == 0) {
		vm->double_class = class;
	}

	if (super != NULL) {
		lit_table_add_all(MM(vm), &class->static_methods, &super->static_methods);
		lit_table_add_all(MM(vm), &class->methods, &super->methods);
		lit_table_add_all(MM(vm), &class->static_fields, &super->static_fields);
		lit_table_add_all(MM(vm), &class->fields, &super->fields);
	}

	return class;
}

LitNativeMethod* lit_vm_define_method(LitVm* vm, LitClass* class, LitResolverNativeMethod* method) {
	LitNativeMethod* m = lit_new_native_method(MM(vm), method->function);
	lit_table_set(MM(vm), method->method.is_static ? &class->static_methods : &class->methods, method->method.name, MAKE_OBJECT_VALUE(m));

	return m;
}

LitClassRegistry* lit_declare_class(LitCompiler* compiler, LitType* type, LitMethodRegistry* methods) {
	LitClassRegistry* registry = reallocate(compiler, NULL, 0, sizeof(LitClassRegistry));

	registry->class = type;
	registry->methods = methods;

	int i = 0;

	LitResolverNativeMethod* mt[256];

	while (methods[i].name != NULL) {
		if (i > 256) {
			printf("Error: more than 256 methods in a native class\n");
			return NULL;
		}

		LitMethodRegistry method = methods[i];
		mt[i] = lit_compiler_define_method(compiler, type, method.name, method.signature, method.fn, method.is_static);

		i++;
	}

	if (i > 0) {
		registry->natives = reallocate(compiler, NULL, 0, sizeof(LitResolverNativeMethod) * i);

		for (int j = 0; j < i; j++) {
			registry->natives[j] = *mt[j];
		}
	} else {
		registry->natives = NULL;
	}

	return registry;
}

void lit_define_class(LitVm* vm, LitClassRegistry* class) {
	LitClass* super = NULL;

	if (class->class->super != NULL) {
		super = AS_CLASS(*lit_table_get(&vm->globals, class->class->super->name));

		if (super == NULL) {
			printf("Creating class error: super %s was not found\n", class->class->super->name->chars);
			return;
		}
	}

	LitClass* object_class = lit_vm_define_class(vm, class->class, super);

	if (class->natives == NULL) {
		return;
	}

	int i = 0;

	while (class->methods[i].name != NULL) {
		lit_vm_define_method(vm, object_class, &class->natives[i]);

		i++;
	}
}

LitNativeRegistry* lit_declare_native(LitCompiler* compiler, LitNativeFn fn, const char* name, const char* signature) {
	LitNativeRegistry* registry = reallocate(compiler, NULL, 0, sizeof(LitNativeRegistry));

	registry->function = fn;
	registry->name = name;
	registry->signature = signature;

	lit_compiler_define_native(compiler, registry);

	return registry;
}

void lit_define_lib(LitVm* vm, LitLibRegistry* lib) {
	if (lib->classes != NULL) {
		int i = 0;

		while (lib->classes[i] != NULL) {
			lit_define_class(vm, lib->classes[i]);
			i++;
		}
	}

	if (lib->functions != NULL) {
		int i = 0;

		while (lib->functions[i] != NULL) {
			lit_vm_define_native(vm, lib->functions[i]);
			i++;
		}
	}
}