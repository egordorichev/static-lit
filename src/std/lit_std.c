#include <std/lit_std.h>
#include <time.h>

int native_object_test(LitVm* vm, LitInstance* instance, LitValue* args, int count) {
	printf("Test method was called!\n");
	return 0;
}

static LitMethodRegistry object_methods[] = {
	{ "test", "Function<void>", native_object_test, false },
	{ NULL } // Null terminator
};

static int time_function(LitVm* vm, LitValue* args, int count) {
	lit_push(vm, MAKE_NUMBER_VALUE((double) clock() / CLOCKS_PER_SEC));
	return 1;
}

static int print_function(LitVm* vm, LitValue* args, int count) {
	printf("%s\n", lit_to_string(vm, args[0]));
	return 0;
}

LitLibRegistry* lit_create_std(LitCompiler* compiler) {
	LitLibRegistry* lib = reallocate(compiler, NULL, 0, sizeof(LitLibRegistry));

	lib->classes = reallocate(compiler, NULL, 0, sizeof(LitClassRegistry) * 2);
	lib->classes[0] = lit_declare_class(compiler, lit_compiler_define_class(compiler, "Object", NULL), object_methods),
	lib->classes[1] = NULL;

	lib->functions = reallocate(compiler, NULL, 0, sizeof(LitNativeRegistry) * 3);
	lib->functions[0] = lit_declare_native(compiler, time_function, "time", "Function<double>");
	lib->functions[1] = lit_declare_native(compiler, print_function, "print", "Function<any, void>");
	lib->functions[2] = NULL;

	return lib;
}