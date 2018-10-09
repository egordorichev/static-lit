#ifndef LIT_TABLE_H
#define LIT_TABLE_H

#include "lit_common.h"
#include "lit_value.h"
#include "lit_vm.h"

#define TABLE_MAX_LOAD 0.75

#define DECLARE_TABLE(name, val, shr) \
	typedef struct { \
		LitString* key; \
		val value; \
	} name##Entry; \
	\
	typedef struct { \
		int count; \
		int capacity_mask; \
		name##Entry* entries; \
	} name; \
	\
	void lit_init_##shr(name* table); \
	void lit_free_##shr(LitVm* vm, name* table); \
	val* lit_##shr##_get(name* table, LitString* key); \
	bool lit_##shr##_set(LitVm* vm, name* table, LitString* key, val value); \
	bool lit_##shr##_delete(LitVm* vm, name* table, LitString* key); \
	LitString* lit_##shr##_find(name* table, const char* chars, int length, uint32_t hash); \
	void lit_##shr##_add_all(LitVm* vm, name* to, name* from); \
	void lit_##shr##_remove_white(LitVm* vm, name* table); \

DECLARE_TABLE(LitTable, LitValue, table)

#define DEFINE_TABLE(name, val, shr, nil) \
	void lit_init_##shr(name* table) { \
		table->count = 0; \
		table->capacity_mask = -1; \
		table->entries = NULL; \
	} \
	\
	void lit_free_##shr(LitVm* vm, name* table) { \
		FREE_ARRAY(vm, name##Entry, table->entries, table->capacity_mask + 1); \
		lit_init_##shr(table); \
	} \
	\
	static uint32_t find_entry_##shr(name##Entry* entries, int capacity_mask, LitString* key) { \
		uint32_t index = key->hash & capacity_mask; \
	\
		for (;;) { \
			name##Entry* entry = &entries[index]; \
	\
			if (entry->key == NULL || entry->key == key) { \
				return index; \
			} \
	\
			index = (index + 1) & capacity_mask; \
		} \
	} \
	\
	val* lit_##shr##_get(name* table, LitString* key) { \
		if (table->entries == NULL) { \
			return NULL; \
		} \
	\
		uint32_t index = find_entry_##shr(table->entries, table->capacity_mask, key); \
		name##Entry* entry = &table->entries[index]; \
	\
		if (entry->key == NULL) { \
			return NULL; \
		} \
	\
		return &entry->value; \
	} \
	\
	static void resize_##shr(LitVm* vm, name* table, int capacity_mask) { \
		name##Entry* entries = ALLOCATE(vm, name##Entry, capacity_mask + 1); \
	\
		for (int i = 0; i <= capacity_mask; i++) { \
			entries[i].key = NULL; \
			entries[i].value = nil; \
		} \
	\
		table->count = 0; \
	\
		for (int i = 0; i <= table->capacity_mask; i++) { \
			name##Entry* entry = &table->entries[i]; \
			if (entry->key == NULL) { \
				continue; \
			} \
	\
			uint32_t index = find_entry_##shr(entries, capacity_mask, entry->key); \
	\
			name##Entry* dest = &entries[index]; \
			dest->key = entry->key; \
			dest->value = entry->value; \
			table->count++; \
		} \
	\
		FREE_ARRAY(vm, val, table->entries, table->capacity_mask + 1); \
	\
		table->entries = entries; \
		table->capacity_mask = capacity_mask; \
	} \
	\
	bool lit_##shr##_set(LitVm* vm, name* table, LitString* key, val value) { \
		if (table->count + 1 > (table->capacity_mask + 1) * TABLE_MAX_LOAD) { \
			int capacity_mask = GROW_CAPACITY(table->capacity_mask + 1) - 1; \
			resize_##shr(vm, table, capacity_mask); \
		} \
	\
		uint32_t index = find_entry_##shr(table->entries, table->capacity_mask, key); \
		name##Entry* entry = &table->entries[index]; \
	\
		bool is_new = entry->key == NULL; \
		entry->key = key; \
		entry->value = value; \
	\
		if (is_new) { \
			table->count++; \
		} \
	\
		return is_new; \
	} \
	\
	bool lit_##shr##_delete(LitVm* vm, name* table, LitString* key) { \
		if (table->count == 0) { \
			return false; \
		} \
	\
		uint32_t index = find_entry_##shr(table->entries, table->capacity_mask, key); \
		name##Entry* entry = &table->entries[index]; \
	\
		if (entry->key == NULL) { \
			return false; \
		} \
	\
		entry->key = NULL; \
		entry->value = nil; \
		table->count--; \
	\
		for (;;) { \
			index = (index + 1) & table->capacity_mask; \
			entry = &table->entries[index]; \
	\
			if (entry->key == NULL) { \
				break; \
			} \
	\
			LitString* temp_key = entry->key; \
			val temp_value = entry->value; \
			entry->key = NULL; \
			entry->value = nil; \
			table->count--; \
	\
			lit_##shr##_set(vm, table, temp_key, temp_value); \
		} \
	\
		return true; \
	} \
	\
	LitString* lit_##shr##_find(name* table, const char* chars, int length, uint32_t hash) { \
		if (table->entries == NULL) { \
			return NULL; \
		} \
	\
		uint32_t index = hash & table->capacity_mask; \
	\
		for (;;) { \
			name##Entry* entry = &table->entries[index]; \
	\
			if (entry->key == NULL) { \
				return NULL; \
			} \
	\
			if (entry->key->length == length && memcmp(entry->key->chars, chars, length) == 0) { \
				return entry->key; \
			} \
	\
			index = (index + 1) & table->capacity_mask; \
		} \
	\
		return NULL; \
	} \
	\
	void lit_##shr##_add_all(LitVm* vm, name* to, name* from) { \
		for (int i = 0; i <= from->capacity_mask; i++) { \
			name##Entry* entry = &from->entries[i]; \
	\
			if (entry->key != NULL) { \
				lit_##shr##_set(vm, to, entry->key, entry->value); \
			} \
		} \
	} \
	\
	void lit_##shr##_remove_white(LitVm* vm, name* table) { \
		for (int i = 0; i <= table->capacity_mask; i++) { \
			name##Entry* entry = &table->entries[i]; \
	\
			if (entry->key != NULL && !entry->key->object.dark) { \
				lit_##shr##_delete(vm, table, entry->key); \
			} \
		} \
	} \


void lit_table_gray(LitVm* vm, LitTable* table);

#endif