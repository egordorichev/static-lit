#ifndef LIT_MEM_MANAGER_H
#define LIT_MEM_MANAGER_H

#include <lit_common.h>
#include <lit_predefines.h>

#include <vm/lit_value.h>
#include <util/lit_table.h>

typedef enum {
	MANAGER_COMPILER,
	MANAGER_VM
} LitMemManagerType;

struct sLitMemManager {
	LitMemManagerType type;
	size_t bytes_allocated;
	LitObject* objects;
	LitTable strings;
} sLitMemManager;

#define MM(x) ((LitMemManager*) x)

#endif