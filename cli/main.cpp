#include <iostream>

#include "lit_context.hpp"

int main(int argc, char **argv) {
	LitContext context;
	context.execute(R"(
if (false) {
 print "It's true!"
} else {
 print "It's false :("
})");

	return 0;
}