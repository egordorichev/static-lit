#include <iostream>

#include "lit_context.hpp"

int main(int argc, char **argv) {
	LitContext context;
	context.execute(R"("yo")");

	return 0;
}