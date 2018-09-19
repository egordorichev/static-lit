#include <string.h>

#include "lit_table.h"
#include "lit_memory.h"
#include "lit_object.h"

#define TABLE_MAX_LOAD 0.75

void lit_init_table(LitTable* table) {
	table->count = 0;
	table->capacity_mask = -1;
	table->entries = NULL;
}

void lit_free_table(LitVm* vm, LitTable* table) {
	FREE_ARRAY(vm, LitTableEntry, table->entries, table->capacity_mask + 1);
	lit_init_table(table);
}

static uint32_t find_entry(LitTableEntry* entries, int capacityMask, LitString* key) {
	uint32_t index = key->hash & capacityMask;

	for (;;) {
		LitTableEntry* entry = &entries[index];

		if (entry->key == NULL || entry->key == key) {
			return index;
		}

		index = (index + 1) & capacityMask;
	}
}

bool lit_table_get(LitTable* table, LitString* key, LitValue* value) {
	if (table->entries == NULL) {
		return false;
	}

	uint32_t index = find_entry(table->entries, table->capacity_mask, key);
	LitTableEntry* entry = &table->entries[index];

	if (entry->key == NULL) {
		return false;
	}

	*value = entry->value;
	return true;
}

static void resize_table(LitVm* vm, LitTable* table, int capacity_mask) {
	LitTableEntry* entries = ALLOCATE(vm, LitTableEntry, capacity_mask + 1);

	for (int i = 0; i <= capacity_mask; i++) {
		entries[i].key = NULL;
		entries[i].value = NIL_VALUE;
	}

	table->count = 0;

	for (int i = 0; i <= table->capacity_mask; i++) {
		LitTableEntry* entry = &table->entries[i];
		if (entry->key == NULL) {
			continue;
		}

		uint32_t index = find_entry(entries, capacity_mask, entry->key);

		LitTableEntry* dest = &entries[index];
		dest->key = entry->key;
		dest->value = entry->value;
		table->count++;
	}

	FREE_ARRAY(vm, LitValue, table->entries, table->capacity_mask + 1);

	table->entries = entries;
	table->capacity_mask = capacity_mask;
}

bool lit_table_set(LitVm* vm, LitTable* table, LitString* key, LitValue value) {
	if (table->count + 1 > (table->capacity_mask + 1) * TABLE_MAX_LOAD) {
		int capacityMask = GROW_CAPACITY(table->capacity_mask + 1) - 1;
		resize_table(vm, table, capacityMask);
	}

	uint32_t index = find_entry(table->entries, table->capacity_mask, key);
	LitTableEntry* entry = &table->entries[index];

	bool is_new = entry->key == NULL;
	entry->key = key;
	entry->value = value;

	if (is_new) {
		table->count++;
	}

	return is_new;
}

bool lit_table_delete(LitVm* vm, LitTable* table, LitString* key) {
	if (table->count == 0) {
		return false;
	}

	uint32_t index = find_entry(table->entries, table->capacity_mask, key);
	LitTableEntry* entry = &table->entries[index];

	if (entry->key == NULL) {
		return false;
	}

	entry->key = NULL;
	entry->value = NIL_VALUE;
	table->count--;

	for (;;) {
		index = (index + 1) & table->capacity_mask;
		entry = &table->entries[index];

		if (entry->key == NULL) {
			break;
		}

		LitString* temp_key = entry->key;
		LitValue temp_value = entry->value;
		entry->key = NULL;
		entry->value = NIL_VALUE;
		table->count--;

		lit_table_set(vm, table, temp_key, temp_value);
	}

	return true;
}

LitString* lit_table_find(LitTable* table, const char* chars, int length, uint32_t hash) {
	if (table->entries == NULL) {
		return NULL;
	}

	uint32_t index = hash & table->capacity_mask;

	for (;;) {
		LitTableEntry* entry = &table->entries[index];

		if (entry->key == NULL) {
			return NULL;
		}

		if (entry->key->length == length && memcmp(entry->key->chars, chars, length) == 0) {
			return entry->key;
		}

		index = (index + 1) & table->capacity_mask;
	}

	return NULL;
}

void lit_table_add_all(LitVm* vm, LitTable* to, LitTable* from) {
	for (int i = 0; i <= from->capacity_mask; i++) {
		LitTableEntry* entry = &from->entries[i];

		if (entry->key != NULL) {
			lit_table_set(vm, to, entry->key, entry->value);
		}
	}
}

void lit_table_remove_white(LitVm* vm, LitTable* table) {
	for (int i = 0; i <= table->capacity_mask; i++) {
		LitTableEntry* entry = &table->entries[i];

		if (entry->key != NULL && !entry->key->object.dark) {
			lit_table_delete(vm, table, entry->key);
		}
	}
}

void lit_table_gray(LitVm* vm, LitTable* table) {
	for (int i = 0; i <= table->capacity_mask; i++) {
		LitTableEntry* entry = &table->entries[i];

		lit_gray_object(vm, (LitObject *) entry->key);
		lit_gray_value(vm, entry->value);
	}
}