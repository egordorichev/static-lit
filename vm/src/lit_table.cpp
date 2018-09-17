#include <cstring>
#include "lit_table.hpp"
#include "lit_value.hpp"
#include "lit_object.hpp"
#include "lit_gc.hpp"

#define TABLE_MAX_LOAD 0.75

static uint32_t find_entry(Entry* entries, int capacityMask, LitString* key) {
	uint32_t index = key->hash & capacityMask;

	for (;;) {
		Entry* entry = &entries[index];

		if (entry->key == nullptr || entry->key == key) {
			return index;
		}

		index = (index + 1) & capacityMask;
	}
}

static void resize(LitTable* table, int capacity_mask) {
	Entry* entries = ALLOCATE(Entry, capacity_mask + 1);

	for (int i = 0; i <= capacity_mask; i++) {
		entries[i].key = nullptr;
		entries[i].value = NIL_VAL;
	}

	table->count = 0;
	for (int i = 0; i <= table->capacity_mask; i++) {
		Entry* entry = &table->entries[i];

		if (entry->key == nullptr) {
			continue;
		}

		uint32_t index = find_entry(entries, capacity_mask, entry->key);
		Entry* dest = &entries[index];
		dest->key = entry->key;
		dest->value = entry->value;
		table->count++;
	}

	FREE_ARRAY(LitValue, table->entries, table->capacity_mask + 1);
	table->entries = entries;
	table->capacity_mask = capacity_mask;
}

LitTable::LitTable() {
	count = 0;
	capacity_mask = -1;
	entries = nullptr;
}

LitTable::~LitTable() {
	FREE_ARRAY(Entry, entries, capacity_mask + 1);
}

bool LitTable::get(LitString* key, LitValue* value) {
	if (entries == nullptr) {
		return false;
	}

	uint32_t index = find_entry(entries, capacity_mask, key);

	Entry* entry = &entries[index];

	if (entry->key == nullptr) {
		return false;
	}

	*value = entry->value;
	return true;
}

bool LitTable::set(LitString* key, LitValue value) {
	if (count + 1 > (capacity_mask + 1) * TABLE_MAX_LOAD) {
		int mask = GROW_CAPACITY(capacity_mask + 1) - 1;
		resize(this, mask);
	}

	uint32_t index = find_entry(entries, capacity_mask, key);
	Entry* entry = &entries[index];

	bool is_new = entry->key == nullptr;

	entry->key = key;
	entry->value = value;

	if (is_new) {
		count++;
	}

	return is_new;
}

bool LitTable::remove(LitString* key) {
	if (count == 0) {
		return false;
	}

	uint32_t index = find_entry(entries, capacity_mask, key);
	Entry* entry = &entries[index];

	if (entry->key == nullptr) {
		return false;
	}

	entry->key = nullptr;
	entry->value = NIL_VAL;
	count--;

	for (;;) {
		index = (index + 1) & capacity_mask;
		entry = &entries[index];

		if (entry->key == nullptr) {
			break;
		}

		LitString* temp_key = entry->key;
		LitValue temp_value = entry->value;

		entry->key = nullptr;
		entry->value = NIL_VAL;
		count--;

		this->set(temp_key, temp_value);
	}

	return true;
}

bool LitTable::add_all(LitTable* table) {
	for (int i = 0; i <= table->capacity_mask; i++) {
		Entry* entry = &table->entries[i];

		if (entry->key != nullptr) {
			this->set(entry->key, entry->value);
		}
	}
}

LitString* LitTable::find(const char* chars, int length, uint32_t hash) {
	if (entries == nullptr) {
		return nullptr;
	}

	uint32_t index = hash & capacity_mask;

	for (;;) {
		Entry* entry = &entries[index];

		if (entry->key == nullptr) {
			return nullptr;
		}

		if (entry->key->length == length && memcmp(entry->key->chars, chars, length) == 0) {
			return entry->key;
		}

		index = (index + 1) & capacity_mask;
	}
}

void LitTable::remove_white() {
	for (int i = 0; i <= capacity_mask; i++) {
		Entry* entry = &entries[i];

		if (entry->key != nullptr && !entry->key->object.dark) {
			this->remove(entry->key);
		}
	}
}

void LitTable::gray() {
	for (int i = 0; i <= capacity_mask; i++) {
		Entry* entry = &entries[i];

		gray_object((LitObject*) entry->key);
		gray_value(entry->value);
	}
}
