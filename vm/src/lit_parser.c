#include "lit_parser.h"

LitExpression *lit_parse(LitVm* vm) {
	return (LitExpression*) lit_make_binary_expression(vm, NULL, NULL, TOKEN_PLUS);
}