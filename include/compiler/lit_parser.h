#ifndef LIT_PARSER_H
#define LIT_PARSER_H

#include <compiler/lit_ast.h>
#include <util/lit_array.h>

bool lit_parse(LitVm* vm, LitStatements* statements);

#endif