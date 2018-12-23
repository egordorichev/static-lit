#include <memory.h>

#include <lit_debug.h>

#include <compiler/lit_compiler.h>
#include <compiler/lit_parser.h>
#include <compiler/lit_resolver.h>

void lit_init_compiler(LitCompiler* compiler) {
	LitMemManager* manager = (LitMemManager*) compiler;

	manager->bytes_allocated = 0;
	manager->type = MANAGER_COMPILER;
	manager->objects = NULL;

	lit_init_table(&manager->strings);

	compiler->init_string = lit_copy_string(manager, "init", 4);
	compiler->resolver.compiler = compiler;
	lit_init_resolver(&compiler->resolver);
	lit_init_resolver_locals(&compiler->resolver.externals);

	lit_init_emitter(compiler, &compiler->emitter);
}

void lit_free_compiler(LitCompiler* compiler) {
	if (DEBUG_TRACE_MEMORY_LEAKS) {
		printf("Bytes allocated after before freeing compiler: %ld\n", compiler->mem_manager.bytes_allocated);
	}

	lit_free_emitter(&compiler->emitter);
	lit_free_resolver(&compiler->resolver);
}

void lit_free_bytecode_objects(LitCompiler* compiler) {
	lit_free_table(MM(compiler), &compiler->mem_manager.strings);
	lit_free_objects(MM(compiler));

	if (DEBUG_TRACE_MEMORY_LEAKS) {
		printf("Bytes allocated after freeing compiler and bytecode: %ld\n", compiler->mem_manager.bytes_allocated);
	}
}

/*
 * Splits source code into tokens and converts them to AST tree
 * Then resolves the AST tree (finds non existing vars, etc)
 * And emits it into bytecode
 */

LitFunction* lit_compile(LitCompiler* compiler, const char* source_code) {
	lit_init_lexer(compiler, &compiler->lexer, source_code);
	LitStatements statements;

	lit_init_statements(&statements);

	if (lit_parse(compiler, &compiler->lexer, &statements)) {
		for (int i = 0; i < statements.count; i++) {
			lit_free_statement(compiler, statements.values[i]);
		}

		lit_free_statements((LitMemManager *) compiler, &statements);
		return NULL; // Parsing error
	}

	if (lit_resolve(compiler, &statements)) {
		for (int i = 0; i < statements.count; i++) {
			lit_free_statement(compiler, statements.values[i]);
		}

		lit_free_statements((LitMemManager *) compiler, &statements);
		return NULL; // Resolving error
	}

	LitFunction* function = lit_emit(&compiler->emitter, &statements);

	if (DEBUG_TRACE_CODE) {
		lit_trace_chunk(MM(compiler), &function->chunk, "$main");
	}

	for (int i = 0; i < statements.count; i++) {
		lit_free_statement(compiler, statements.values[i]);
	}

	lit_free_statements((LitMemManager *) compiler, &statements);
	return function;
}

void lit_compiler_define_native(LitCompiler* compiler, LitNativeRegistry* native) {
	LitString* str = lit_copy_string(MM(compiler), native->name, (int) strlen(native->name));
	LitResolverLocal* letal = (LitResolverLocal*) reallocate(compiler, NULL, 0, sizeof(LitResolverLocal));

	size_t len = strlen(native->signature);
	char* tp = (char*) reallocate(compiler, NULL, 0, len);
	strncpy(tp, native->signature, len);

	letal->type = tp;
	letal->defined = true;
	letal->nil = false;
	letal->field = false;

	lit_resolver_locals_set(MM(compiler), &compiler->resolver.externals, str, letal);
}

void lit_compiler_define_natives(LitCompiler* compiler, LitNativeRegistry* natives) {
	int i = 0;
	LitNativeRegistry native;

	do {
		native = natives[i++];

		if (native.name != NULL) {
			lit_compiler_define_native(compiler, &native);
		}
	} while (native.name != NULL);
}

LitType* lit_compiler_define_class(LitCompiler* compiler, const char* name, LitType* super) {
	LitType* type = reallocate(compiler, NULL, 0, sizeof(LitType));
	lit_init_type(type);

	type->name = lit_copy_string(MM(compiler), name, strlen(name));
	type->super = super;
	type->external = true;

	lit_classes_set(MM(compiler), &compiler->resolver.classes, type->name, type);
	lit_types_set(MM(compiler), &compiler->resolver.types, type->name, true);

	LitResolverLocal* local = (LitResolverLocal*) reallocate(compiler, NULL, 0, sizeof(LitResolverLocal));
	lit_init_resolver_local(local);

	local->defined = true;
	local->type = lit_format_string(MM(compiler), "Class<$>", type->name->chars)->chars;

	lit_resolver_locals_set(MM(compiler), compiler->resolver.scopes.values[0], type->name, local);

	if (super != NULL) {
		lit_resolver_methods_add_all(MM(compiler), &type->methods, &super->methods);
		lit_resolver_fields_add_all(MM(compiler), &type->fields, &super->fields);
	}

	if (compiler->resolver.int_class == NULL && strcmp(name, "Int") == 0) {
		compiler->resolver.int_class = type;
	} else 	if (compiler->resolver.string_class == NULL && strcmp(name, "String") == 0) {
		compiler->resolver.string_class = type;
	} else 	if (compiler->resolver.double_class == NULL && strcmp(name, "Double") == 0) {
		compiler->resolver.double_class = type;
	} else 	if (compiler->resolver.bool_class == NULL && strcmp(name, "Bool") == 0) {
		compiler->resolver.bool_class = type;
	}

	return type;
}

LitResolverNativeMethod* lit_compiler_define_method(LitCompiler* compiler, LitType* class, const char* name, const char* signature, LitNativeMethodFn method, bool is_static) {
	LitResolverNativeMethod* m = (LitResolverNativeMethod*) reallocate(compiler, NULL, 0, sizeof(LitResolverNativeMethod));

	m->function = method;
	LitResolverMethod* mt = (LitResolverMethod*) m;

	mt->signature = (char *) signature;
	mt->is_static = is_static;
	mt->access = PUBLIC_ACCESS;
	mt->abstract = false;
	mt->is_overriden = false;
	mt->name = lit_copy_string(MM(compiler), name, strlen(name));

	lit_resolver_methods_set(MM(compiler), mt->is_static ? &class->static_methods : &class->methods, lit_copy_string(MM(compiler), name, strlen(name)), mt);

	return m;
}