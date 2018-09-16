#include <iostream>

#include "lit.hpp"
#include "debug.hpp"
#include "vm.hpp"

int main(int argc, char **argv) {
	const char *source_code = "1 + 1";
	lit_execute(source_code);

	return 0;
}