#ifndef LIT_PARSER_H
#define LIT_PARSER_H

/*
 * Parser converts token stream into AST
 */

#include <compiler/lit_ast.h>
#include <compiler/lit_compiler.h>
#include <util/lit_array.h>

bool lit_parse(LitCompiler* compiler, LitLexer* lexer, LitStatements* statements);

#endif