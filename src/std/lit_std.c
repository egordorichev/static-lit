#include <std/lit_std.h>

int native_object_test(LitVm* vm, LitInstance* instance, LitValue* args, int count) {
	printf("Test method was called!\n");
	return 0;
}

static LitMethodRegistry object_methods[] = {
	{ "test", "Function<void>", native_object_test, false },

	{ NULL } // Null terminator
};

LitClassRegistry* lit_create_std(LitCompiler* compiler) {
	LitType* object_class = lit_compiler_define_class(compiler, "Object", NULL);

	return lit_declare_class(compiler, object_class, object_methods);
}