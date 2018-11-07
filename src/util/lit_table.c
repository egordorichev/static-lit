#include <string.h>

#include <util/lit_table.h>
#include <vm/lit_memory.h>
#include <vm/lit_object.h>

DEFINE_TABLE(LitTable, LitValue, table, LitValue*, NIL_VALUE, &entry->value);

void lit_table_gray(LitVm* vm, LitTable* table) {
	for (int i = 0; i <= table->capacity_mask; i++) {
		LitTableEntry* entry = &table->entries[i];

		lit_gray_object(vm, (LitObject *) entry->key);
		lit_gray_value(vm, entry->value);
	}
}