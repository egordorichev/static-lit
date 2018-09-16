#include <iostream>

#include "lit.hpp"
#include "vm.hpp"

int main(int argc, char **argv) {
	const char *source_code = "1 + 1";
	LitState state;
	state.execute(source_code);

	return 0;
}