#include <iostream>

#include "lit_context.hpp"

int main(int argc, char **argv) {
	LitContext context;
	context.execute("!(5 - 4 > 3 * 2 == !nil)");

	return 0;
}