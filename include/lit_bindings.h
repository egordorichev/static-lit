#ifndef LIT_BINDINGS_H
#define LIT_BINDINGS_H

/*
 * Provides a nice way to define
 * native functions and classes
 */

#define START_LIB LitLibRegistry* lib = reallocate(compiler, NULL, 0, sizeof(LitLibRegistry));
#define END_LIB return lib;

#define START_CLASSES(size) ({\
	int i = 0; \
	lib->classes = reallocate(compiler, NULL, 0, sizeof(LitClassRegistry) * ((size) + 1)); \

#define END_CLASSES lib->classes[i] = NULL; \
	});

#define DEFINE_CLASS(name, id, super) \
	LitType* id##_class = (lib->classes[i] = \
	lit_declare_class(compiler, lit_compiler_define_class(compiler, name, super), id##_methods))->class; \
	i++;

#define METHOD(name) int name(LitVm* vm, LitValue instance, const LitValue* args, int count)
#define START_METHODS(name) static LitMethodRegistry name##_methods[] = {
#define ADD(name, signature, fn, stat) { name, signature, fn, stat },
#define END_METHODS { NULL, NULL, NULL, NULL } };

#define START_FUNCTIONS(size) ({\
	int i = 0; \
	lib->functions = reallocate(compiler, NULL, 0, sizeof(LitNativeRegistry) * ((size) + 1)); \

#define END_FUNCTIONS lib->functions[i] = NULL; \
	});

#define DEFINE_FUNCTION(function, name, signature)\
	lib->functions[i] = lit_declare_native(compiler, function, name, signature);\
	i++;

#define FUNCTION(name) int name##_native(LitVm* vm, LitValue* args, int count)
#define RETURN_VOID return 0;
#define RETURN_NUMBER(number) lit_push(vm, MAKE_NUMBER_VALUE(number)); return 1;
#define RETURN_NIL lit_push(vm, NIL_VALUE); return 1;
#define RETURN_BOOL(value) lit_push(vm, MAKE_BOOL_VALUE(value)); return 1;
#define RETURN_OBJECT(value) lit_push(vm, MAKE_OBJECT_VALUE(value)); return 1;
#define RETURN_STRING(value) lit_push(vm, MAKE_OBJECT_VALUE(value)); return 1;

#endif