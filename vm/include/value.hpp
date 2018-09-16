#ifndef LIT_VALUE_HPP
#define LIT_VALUE_HPP

#include "config.hpp"

#define LitValue LitNumber

class LitValueArray {
	public:
		LitValueArray();
		~LitValueArray();

		void write(LitValue value);
		void free();

		int get_count() { return count; }
		int get_capacity() { return capacity; }
		LitValue get(int i) { return values[i]; }
	private:
		int count;
		int capacity;
		LitValue *values;
};

void lit_print_value(LitValue value);

#endif