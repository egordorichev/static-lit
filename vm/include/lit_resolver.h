#ifndef LIT_RESOLVER_H
#define LIT_RESOLVER_H

#include "lit_common.h"
#include "lit_ast.h"
#include "lit_table.h"
#include "lit_vm.h"
#include "lit_object.h"

typedef struct LitLetal {
	bool defined;
	bool nil;
	bool field;
	const char* type;
} LitLetal;

void lit_init_letal(LitLetal* letal);

DECLARE_TABLE(LitLetals, LitLetal, letals)
DECLARE_TABLE(LitTypes, char *, types)
DECLARE_TABLE(LitClasses, LitClass*, classes)
DECLARE_ARRAY(LitScopes, LitLetals*, scopes)

typedef struct LitResolver {
	LitScopes scopes;
	LitLetals externals;
	LitTypes types;
	LitClasses classes;
	LitVm* vm;
	int depth;
	LitClass* class;

	bool had_return;
	bool had_error;

	LitFunctionStatement* function;
} LitResolver;

void lit_init_resolver(LitResolver* resolver);
void lit_free_resolver(LitResolver* resolver);

bool lit_resolve(LitVm* vm, LitStatements* statements);

#endif