#ifndef LIT_RESOLVER_H
#define LIT_RESOLVER_H

#include <lit_common.h>
#include <lit_predefines.h>

#include <compiler/lit_ast.h>
#include <util/lit_table.h>

typedef struct LitLetal {
	bool defined;
	bool nil;
	bool field;
	const char* type;
} LitLetal;

void lit_init_letal(LitLetal* letal);

typedef struct LitResource {
	bool is_static;
	bool is_final;
	LitAccessType access;
	const char* type;
} LitResource;

void lit_free_resource(LitCompiler* compiler, LitResource* resource);

DECLARE_TABLE(LitResources, LitResource*, resources, LitResource*)

typedef struct LitRem {
	bool is_static;
	bool is_overriden;
	LitAccessType access;
	char* signature;
} LitRem;

void lit_free_rem(LitCompiler* compiler, LitRem* rem);

DECLARE_TABLE(LitRems, LitRem*, rems, LitRem*)

typedef struct sLitType {
	LitString* name;
	struct sLitType* super;
	LitRems methods;
	LitRems static_methods;
	LitResources fields;
} LitType;

void lit_init_type(LitType* type);
void lit_free_type(LitCompiler* compiler, LitType* type);

DECLARE_TABLE(LitLetals, LitLetal*, letals, LitLetal*)
DECLARE_TABLE(LitTypes, bool, types, bool)
DECLARE_TABLE(LitClasses, LitType*, classes, LitType*)
DECLARE_ARRAY(LitScopes, LitLetals*, scopes)
DECLARE_ARRAY(LitStrings, char*, strings)

typedef struct LitResolver {
	LitScopes scopes;
	LitLetals externals;
	LitTypes types;
	LitStrings allocated_strings;
	LitClasses classes;

	LitCompiler* compiler;

	int depth;
	LitType* class;

	bool had_return;
	bool had_error;

	LitFunctionStatement* function;
} LitResolver;

void lit_init_resolver(LitResolver* resolver);
void lit_free_resolver(LitResolver* resolver);

bool lit_resolve(LitCompiler* compiler, LitStatements* statements);

#endif