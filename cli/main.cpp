#include <iostream>

#include "lit_context.hpp"

int main(int argc, char **argv) {
	LitContext context;
	context.execute("\"Hello, world!\" + \" Hi\"");

	return 0;
}