#ifndef LIT_TABLE_HPP
#define LIT_TABLE_HPP

#include "lit_common.hpp"
#include "lit_value.hpp"

typedef struct {
  LitString* key;
  LitValue value;
} Entry;

class LitTable {
	public:
		int count;
		int capacity_mask;
		Entry* entries;

	public:
	  LitTable();
	  ~LitTable();

	  bool get(LitString* key, LitValue* value);
	  bool set(LitString* key, LitValue value);
	  bool remove(LitString* key);
	  bool add_all(LitTable* table);
	  LitString* find(const char* chars, int length, uint32_t hash);
	  void remove_white();
	  // void gray(LitVm* vm);
};

#endif