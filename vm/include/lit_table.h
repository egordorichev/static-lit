#ifndef LIT_TABLE_H
#define LIT_TABLE_H

#include "lit_common.h"
#include "lit_value.h"
#include "lit_vm.h"

typedef struct {
	LitString* key;
	LitValue value;
} LitTableEntry;

typedef struct {
	int count;
	int capacity_mask;
	LitTableEntry* entries;
} LitTable;

void lit_init_table(LitTable* table);
void lit_free_table(LitVm* vm, LitTable* table);

bool lit_table_get(LitTable* table, LitString* key, LitValue* value);
bool lit_table_set(LitVm* vm, LitTable* table, LitString* key, LitValue value);
bool lit_table_delete(LitVm* vm, LitTable* table, LitString* key);
LitString* lit_table_find(LitTable* table, const char* chars, int length, uint32_t hash);
void lit_table_add_all(LitVm* vm, LitTable* to, LitTable* from);
void lit_table_remove_white(LitVm* vm, LitTable* table);
void lit_table_gray(LitVm* vm, LitTable* table);

#endif