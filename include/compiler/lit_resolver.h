#ifndef LIT_RESOLVER_H
#define LIT_RESOLVER_H

/*
 * Goes through AST and makes sure,
 * that every accessed variable is defined,
 * and all declarations are right
 */

#include <lit_common.h>
#include <lit_predefines.h>

#include <compiler/lit_ast.h>
#include <util/lit_table.h>
#include <vm/lit_object.h>

typedef struct LitResolverLocal {
	bool defined;
	bool nil;
	bool field;
	bool final;
	const char* type;
} LitResolverLocal;

void lit_init_resolver_local(LitResolverLocal* letal);

typedef struct LitResolverField {
	bool is_static;
	bool is_final;
	LitAccessType access;
	const char* type;
	struct sLitType* original;
} LitResolverField;

void lit_free_resolver_field(LitCompiler* compiler, LitResolverField* resource);

DECLARE_TABLE(LitResolverFields, LitResolverField*, resolver_fields, LitResolverField*)

typedef struct LitResolverMethod {
	bool is_static;
	bool is_overriden;
	bool abstract;
	LitAccessType access;
	char* signature;
	LitString* name;
	struct sLitType* original;
} LitResolverMethod;

void lit_free_resolver_method(LitCompiler* compiler, LitResolverMethod* method);

typedef struct LitResolverNativeMethod {
	LitResolverMethod method;
	LitNativeMethodFn function;
} LitResolverNativeMethod;

DECLARE_TABLE(LitResolverMethods, LitResolverMethod*, resolver_methods, LitResolverMethod*)

typedef struct sLitType {
	LitString* name;
	bool is_static;
	bool final;
	bool abstract;
	bool inited;
	bool external;
	struct sLitType* super;
	LitResolverMethods methods;
	LitResolverMethods static_methods;
	LitResolverFields fields;
	LitResolverFields static_fields;
} LitType;

void lit_init_type(LitType* type);
void lit_free_type(LitMemManager* manager, LitType* type);

DECLARE_TABLE(LitResolverLocals, LitResolverLocal*, letals, LitResolverLocal*)
DECLARE_TABLE(LitTypes, bool, types, bool)
DECLARE_TABLE(LitClasses, LitType*, classes, LitType*)
DECLARE_ARRAY(LitScopes, LitResolverLocals*, scopes)
DECLARE_ARRAY(LitStrings, char*, strings)

typedef struct LitResolver {
	LitScopes scopes;
	LitResolverLocals externals;
	LitTypes types;
	LitStrings allocated_strings;
	LitClasses classes;
	LitStatement* loop;
	LitCompiler* compiler;

	int depth;
	LitType* class;

	bool had_return;
	bool had_error;

	LitFunctionStatement* function;
} LitResolver;

void lit_init_resolver(LitResolver* resolver);
void lit_free_resolver(LitResolver* resolver);
void lit_define_type(LitResolver* resolver, const char* type);

bool lit_resolve(LitCompiler* compiler, LitStatements* statements);

#endif