#include <stdio.h>

#include <lit_debug.h>

#include <vm/lit_object.h>
#include <vm/lit_value.h>

void lit_trace_statement(LitMemManager* manager, LitStatement* statement, int depth) {
	printf("{\n");

	if (statement != NULL) {
		switch (statement->type) {
			case EXPRESSION_STATEMENT: {
				LitExpressionStatement* expression = (LitExpressionStatement*) statement;

				printf("\"type\" : \"expression\",\n");
				printf("\"expression\" : ");
				lit_trace_expression(manager, expression->expr, depth + 1);
				printf("\n");

				break;
			}
			case VAR_STATEMENT: {
				LitVarStatement* var = (LitVarStatement*) statement;

				printf("\"type\" : \"var declaration\",\n");
				printf("\"var_type\" : \"%s\",\n", var->type == NULL ? "undefined" : var->type);
				printf("\"name\" : \"%s\",\n", var->name);
				printf("\"final\" : \"%s\",\n", var->final ? "true" : "false");
				printf("\"init\" : ");

				if (var->init != NULL) {
					lit_trace_expression(manager, var->init, depth + 1);
				} else {
					printf("[]");
				}

				printf("\n");
				break;
			}
			case IF_STATEMENT: {
				LitIfStatement* if_statement = (LitIfStatement*) statement;

				printf("\"type\" : \"if\",\n");
				printf("\"condition\" : ");
				lit_trace_expression(manager, if_statement->condition, depth + 1);
				printf(",\n");

				printf("\"if_branch\" : ");
				lit_trace_statement(manager, if_statement->if_branch, depth + 1);

				printf(",\n\"else_if_branches\" : ");

				if (if_statement->else_if_branches == NULL) {
					printf("[]\n");
				} else {
					printf("[\n");

					int cn = if_statement->else_if_branches->count;

					for (int i = 0; i < cn; i++) {
						printf("{\n\"condition\" : ");
						lit_trace_expression(manager, if_statement->else_if_conditions->values[i], depth + 1);
						printf(",\n\"body\" : ");
						lit_trace_statement(manager, if_statement->else_if_branches->values[i], depth + 1);

						if (i < cn - 1) {
							printf("},\n");
						} else {
							printf("}\n");
						}
					}

					printf("]\n");
				}

				printf(",\n\"else_branch\" : ");

				if (if_statement->else_branch != NULL) {
					lit_trace_statement(manager, if_statement->else_branch, depth + 1);
				} else {
					printf("{}");
				}

				printf("\n");
				break;
			}
			case BLOCK_STATEMENT: {
				LitBlockStatement* block = (LitBlockStatement*) statement;

				printf("\"type\" : \"block\",\n");
				printf("\"statements\" : [");

				int cn = block->statements->count;

				if (cn > 0) {
					printf("\n");

					for (int i = 0; i < cn; i++) {
						lit_trace_statement(manager, block->statements->values[i], depth + 1);

						if (i < cn - 1) {
							printf(",\n");
						} else {
							printf("\n");
						}
					}
				}

				printf("]\n");
				break;
			}
			case WHILE_STATEMENT: {
				LitWhileStatement* while_statement = (LitWhileStatement*) statement;

				printf("\"type\" : \"while\",\n");
				printf("\"condition\" : ");
				lit_trace_expression(manager, while_statement->condition, depth + 1);
				printf(",\n");

				printf("\"body\" : ");
				lit_trace_statement(manager, while_statement->body, depth + 1);

				printf("\n");
				break;
			}
			case FUNCTION_STATEMENT: {
				LitFunctionStatement* function = (LitFunctionStatement*) statement;

				printf("\"type\" : \"function\",\n");
				printf("\"name\" : \"%s\",\n", function->name);
				printf("\"return_type\" : \"%s\",\n", function->return_type.type);
				printf("\"args\" : [");

				if (function->parameters != NULL) {
					printf("\n");
					int cn = function->parameters->count;

					for (int i = 0; i < cn; i++) {
						LitParameter parameter = function->parameters->values[i];

						printf("{\n\"name\" : \"%s\",\n", parameter.name);
						printf("\"type\" : \"%s\"\n}", parameter.type);

						if (i < cn - 1) {
							printf(",\n");
						} else {
							printf("\n");
						}
					}
				}

				printf("],\n\"body\" : ");
				lit_trace_statement(manager, function->body, depth + 1);

				printf("\n");
				break;
			}
			case METHOD_STATEMENT: {
				LitMethodStatement* function = (LitMethodStatement*) statement;

				printf("\"type\" : \"method\",\n");
				printf("\"name\" : \"%s\",\n", function->name);
				printf("\"overriden\" : \"%s\",\n", function->overriden ? "true" : "false");
				printf("\"static\" : \"%s\",\n", function->is_static ? "true" : "false");
				printf("\"abstract\" : \"%s\",\n", function->abstract ? "true" : "false");
				printf("\"access\" : \"%s\",\n", function->access == PUBLIC_ACCESS ? "public" : (function->access == PRIVATE_ACCESS ? "private" : "protected"));
				printf("\"return_type\" : \"%s\",\n", function->return_type.type);
				printf("\"args\" : [");

				if (function->parameters != NULL) {
					printf("\n");
					int cn = function->parameters->count;

					for (int i = 0; i < cn; i++) {
						LitParameter parameter = function->parameters->values[i];

						printf("{\n\"name\" : \"%s\",\n", parameter.name);
						printf("\"type\" : \"%s\"\n}", parameter.type);

						if (i < cn - 1) {
							printf(",\n");
						} else {
							printf("\n");
						}
					}
				}

				printf("],\n\"body\" : ");
				lit_trace_statement(manager, function->body, depth + 1);

				printf("\n");
				break;
			}
			case RETURN_STATEMENT: {
				LitReturnStatement* return_statement = (LitReturnStatement*) statement;

				printf("\"type\" : \"return\",\n");
				printf("\"value\" : ");

				if (return_statement->value == NULL) {
					printf("[]\n");
				} else {
					lit_trace_expression(manager, return_statement->value, depth + 1);
					printf("\n");
				}

				break;
			}
			case CLASS_STATEMENT: {
				LitClassStatement* class = (LitClassStatement*) statement;

				printf("\"type\" : \"class\",\n");
				printf("\"name\" : \"%s\",\n", class->name);
				printf("\"super\" : ");

				if (class->super == NULL) {
					printf("[],\n");
				} else {
					lit_trace_expression(manager, (LitExpression*) class->super, depth + 1);
					printf(",\n");
				}

				printf("\"fields\" : [");

				if (class->fields != NULL) {
					int cn = class->fields->count;
					printf("\n");

					for (int i = 0; i < cn; i++) {
						lit_trace_statement(manager, class->fields->values[i], depth + 1);

						if (i < cn - 1) {
							printf(",\n");
						} else {
							printf("\n");
						}
					}
				}

				printf("],\n");

				printf("\"methods\" : [");

				if (class->methods != NULL) {
					int cn = class->methods->count;
					printf("\n");

					for (int i = 0; i < cn; i++) {
						lit_trace_statement(manager, (LitStatement*) class->methods->values[i], depth + 1);

						if (i < cn - 1) {
							printf(",\n");
						} else {
							printf("\n");
						}
					}
				}

				printf("]\n");
				break;
			}
			case FIELD_STATEMENT: {
				LitFieldStatement* field = (LitFieldStatement*) statement;

				printf("\"type\" : \"field\",\n");
				printf("\"field_type\" : \"%s\",\n", field->type == NULL ? "undefined" : field->type);
				printf("\"name\" : \"%s\",\n", field->name);
				printf("\"static\" : \"%s\",\n", field->is_static ? "true" : "false");
				printf("\"final\" : \"%s\",\n", field->final ? "true" : "false");
				printf("\"access\" : \"%s\",\n", field->access == PUBLIC_ACCESS ? "public" :
					(field->access == PROTECTED_ACCESS ? "protected" : "private"));

				printf("\"init\" : ");

				if (field->init != NULL) {
					lit_trace_expression(manager, field->init, depth + 1);
				} else {
					printf("[]");
				}

				printf(",\n");
				printf("\"getter\" : ");

				if (field->getter != NULL) {
					lit_trace_statement(manager, field->getter, depth + 1);
				} else {
					printf("[]");
				}

				printf(",\n");
				printf("\"setter\" : ");

				if (field->setter != NULL) {
					lit_trace_statement(manager, field->setter, depth + 1);
				} else {
					printf("[]");
				}

				printf(",\n");
				break;
			}
			case BREAK_STATEMENT: {
				printf("\"type\" : \"break\"\n");
				break;
			}
			case CONTINUE_STATEMENT: {
				printf("\"type\" : \"continue\"\n");
				break;
			}
			default: {
				printf("Statement with id %i has no pretty-printer!\n", statement->type);
				UNREACHABLE();
			}
		}
	}

	if (depth == 0) {
		printf("}\n");
	} else {
		printf("}");
	}
}

void lit_trace_expression(LitMemManager* manager, LitExpression* expression, int depth) {
	printf("{\n");

	if (expression != NULL) {
		switch (expression->type) {
			case BINARY_EXPRESSION: {
				LitBinaryExpression* binary = (LitBinaryExpression*) expression;

				printf("\"type\" : \"binary\",\n");
				printf("\"left\" : ");
				lit_trace_expression(manager, binary->left, depth + 1);
				printf(",\n\"right\" : ");
				lit_trace_expression(manager, binary->right, depth + 1);
				printf(",\n\"operator\" : ");

				switch (binary->operator) {
					case TOKEN_MINUS: printf("\"-\"\n"); break;
					case TOKEN_PLUS: printf("\"+\"\n"); break;
					case TOKEN_SLASH: printf("\"/\"\n"); break;
					case TOKEN_STAR: printf("\"*\"\n"); break;
					case TOKEN_LESS: printf("\"<\"\n"); break;
					case TOKEN_LESS_EQUAL: printf("\"<=\"\n"); break;
					case TOKEN_GREATER: printf("\">\"\n"); break;
					case TOKEN_GREATER_EQUAL: printf("\">=\"\n"); break;
				}

				break;
			}
			case LOGICAL_EXPRESSION: {
				LitLogicalExpression* logic = (LitLogicalExpression*) expression;

				printf("\"type\" : \"logic\",\n");
				printf("\"left\" : ");
				lit_trace_expression(manager, logic->left, depth + 1);
				printf(",\n\"right\" : ");
				lit_trace_expression(manager, logic->right, depth + 1);
				printf(",\n\"operator\" : ");

				switch (logic->operator) {
					case TOKEN_AND: printf("\"and\"\n"); break;
					case TOKEN_OR: printf("\"or\"\n"); break;
				}

				break;
			}
			case LITERAL_EXPRESSION: {
				LitLiteralExpression* literal = (LitLiteralExpression*) expression;

				printf("\"type\" : \"literal\",\n");
				printf("\"value\" : \"%s\"\n", lit_to_string(manager, literal->value));
				break;
			}
			case UNARY_EXPRESSION: {
				LitUnaryExpression* unary = (LitUnaryExpression*) expression;

				printf("\"type\" : \"unary\",\n");
				printf("\"right\" : ");
				lit_trace_expression(manager, unary->right, depth + 1);
				printf(",\n\"operator\" : ");

				switch (unary->operator) {
					case TOKEN_MINUS: printf("\"-\"\n"); break;
					case TOKEN_BANG: printf("\"!\"\n"); break;
					default: printf("\"unknown\"\n"); break;
				}

				break;
			}
			case GROUPING_EXPRESSION: {
				LitGroupingExpression* grouping = (LitGroupingExpression*) expression;

				printf("\"type\" : \"grouping\",\n");
				printf("\"expression\" : ");
				lit_trace_expression(manager, grouping->expr, depth + 1);
				printf("\n");

				break;
			}
			case VAR_EXPRESSION: {
				LitVarExpression* var = (LitVarExpression*) expression;

				printf("\"type\" : \"var usage\",\n");
				printf("\"name\" : \"%s\"\n", var->name);

				break;
			}
			case ASSIGN_EXPRESSION: {
				LitAssignExpression* var = (LitAssignExpression*) expression;

				printf("\"type\" : \"assign\",\n");
				printf("\"to\" : ");
				lit_trace_expression(manager, var->to, depth + 1);
				printf("\n");
				printf("\"value\" : ");
				lit_trace_expression(manager, var->value, depth + 1);
				printf("\n");

				break;
			}
			case CALL_EXPRESSION: {
				LitCallExpression* call = (LitCallExpression*) expression;

				printf("\"type\" : \"call\",\n");
				printf("\"callee\" : ");
				lit_trace_expression(manager, call->callee, depth + 1);
				printf(",\n\"args\" : [");

				if (call->args->count > 0) {
					printf("\n");
					int cn = call->args->count;

					for (int i = 0; i < cn; i++) {
						lit_trace_expression(manager, call->args->values[i], depth + 1);

						if (i < cn - 1) {
							printf(",\n");
						} else {
							printf("\n");
						}
					}
				}

				printf("]\n");

				break;
			}
			case GET_EXPRESSION: {
				LitGetExpression* expr = (LitGetExpression*) expression;

				printf("\"type\" : \"get\",\n");
				printf("\"object\" : ");

				lit_trace_expression(manager, expr->object, depth);

				printf(",\n");
				printf("\"property\" : \"%s\"\n", expr->property);

				break;
			}
			case SET_EXPRESSION: {
				LitSetExpression* expr = (LitSetExpression*) expression;

				printf("\"type\" : \"set\",\n");
				printf("\"object\" : ");

				lit_trace_expression(manager, expr->object, depth);

				printf(",\n");
				printf("\"property\" : \"%s\"\n", expr->property);

				break;
			}
			case LAMBDA_EXPRESSION: {
				LitLambdaExpression* lamba = (LitLambdaExpression*) expression;

				printf("\"type\" : \"lambda\",\n");
				printf("\"return_type\" : \"%s\",\n", lamba->return_type.type);
				printf("\"args\" : [");

				if (lamba->parameters != NULL) {
					printf("\n");
					int cn = lamba->parameters->count;

					for (int i = 0; i < cn; i++) {
						LitParameter parameter = lamba->parameters->values[i];

						printf("{\n\"name\" : \"%s\",\n", parameter.name);
						printf("\"type\" : \"%s\"\n}", parameter.type);

						if (i < cn - 1) {
							printf(",\n");
						} else {
							printf("\n");
						}
					}
				}

				printf("],\n\"body\" : ");
				lit_trace_statement(manager, lamba->body, depth + 1);

				printf("\n");
				break;
			}
			case THIS_EXPRESSION: {
				printf("\"type\" : \"this\"\n");
				break;
			}
			case SUPER_EXPRESSION: {
				printf("\"type\" : \"super\",\n");
				printf("\"method\" : \"%s\"\n", ((LitSuperExpression*) expression)->method);
				break;
			}
			default: {
				printf("Expression with id %i has no pretty-printer!\n", expression->type);
				UNREACHABLE();
			}
		}
	}

	if (depth == 0) {
		printf("}\n");
	} else {
		printf("}");
	}
}

void lit_trace_chunk(LitMemManager* manager, LitChunk* chunk, const char* name) {
	printf("== %s ==\n", name);

	for (int i = 0; i < chunk->count;) {
		i = lit_disassemble_instruction(manager, chunk, i);
	}
}

static int simple_instruction(const char* name, int offset) {
	printf("%s\n", name);
	return offset + 1;
}

static int constant_instruction(LitMemManager* manager, const char* name, LitChunk* chunk, int offset) {
	uint8_t constant = chunk->code[offset + 1];
	printf("%-16s %4d '%s'\n", name, constant, lit_to_string(manager, chunk->constants.values[constant]));
	return offset + 2;
}

static int byte_instruction(const char* name, LitChunk* chunk, int offset) {
	uint8_t slot = chunk->code[offset + 1];
	printf("%-16s %4d\n", name, slot);
	return offset + 2;
}

static int jump_instruction(const char* name, int sign, LitChunk* chunk, int offset) {
	uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
	jump |= chunk->code[offset + 2];
	printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
	return offset + 3;
}

int lit_disassemble_instruction(LitMemManager* manager, LitChunk* chunk, int offset) {
	printf("%04d ", offset);
	uint8_t instruction = chunk->code[offset];

	switch (instruction) {
		case OP_RETURN: return simple_instruction("OP_RETURN", offset);
		case OP_ADD: return simple_instruction("OP_ADD", offset);
		case OP_POP: return simple_instruction("OP_POP", offset);
		case OP_SUBTRACT: return simple_instruction("OP_SUBTRACT", offset);
		case OP_MULTIPLY: return simple_instruction("OP_MULTIPLY", offset);
		case OP_DIVIDE: return simple_instruction("OP_DIVIDE", offset);
		case OP_PRINT: return simple_instruction("OP_PRINT", offset);
		case OP_NEGATE: return simple_instruction("OP_NEGATE", offset);
		case OP_NOT: return simple_instruction("OP_NOT", offset);
		case OP_NIL: return simple_instruction("OP_NIL", offset);
		case OP_TRUE: return simple_instruction("OP_TRUE", offset);
		case OP_FALSE: return simple_instruction("OP_FALSE", offset);
		case OP_EQUAL: return simple_instruction("OP_EQUAL", offset);
		case OP_LESS: return simple_instruction("OP_LESS", offset);
		case OP_GREATER: return simple_instruction("OP_GREATER", offset);
		case OP_NOT_EQUAL: return simple_instruction("OP_NOT_EQUAL", offset);
		case OP_GREATER_EQUAL: return simple_instruction("OP_GREATER_EQUAL", offset);
		case OP_LESS_EQUAL: return simple_instruction("OP_LESS_EQUAL", offset);
		case OP_CALL: return simple_instruction("OP_CALL", offset) + 1;
		case OP_DEFINE_GLOBAL: return constant_instruction(manager, "OP_DEFINE_GLOBAL", chunk, offset);
		case OP_GET_GLOBAL: return constant_instruction(manager, "OP_GET_GLOBAL", chunk, offset);
		case OP_SET_GLOBAL: return constant_instruction(manager, "OP_SET_GLOBAL", chunk, offset);
		case OP_GET_LOCAL: return byte_instruction("OP_GET_LOCAL", chunk, offset);
		case OP_SET_LOCAL: return byte_instruction("OP_SET_LOCAL", chunk,offset);
		case OP_GET_UPVALUE: return byte_instruction("OP_GET_UPVALUE", chunk, offset);
		case OP_SET_UPVALUE: return byte_instruction("OP_SET_UPVALUE", chunk, offset);
		case OP_CONSTANT: return constant_instruction(manager, "OP_CONSTANT", chunk, offset);
		case OP_JUMP: return jump_instruction("OP_JUMP", 1, chunk, offset);
		case OP_JUMP_IF_FALSE: return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
		case OP_LOOP: return jump_instruction("OP_LOOP", -1, chunk, offset);
		case OP_CLASS: return constant_instruction(manager, "OP_CLASS", chunk, offset);
		case OP_SUBCLASS: return constant_instruction(manager, "OP_SUBCLASS", chunk, offset);
		case OP_METHOD: return constant_instruction(manager, "OP_METHOD", chunk, offset);
		case OP_GET_FIELD: return constant_instruction(manager, "OP_GET_FIELD", chunk, offset);
		case OP_SET_FIELD: return constant_instruction(manager, "OP_SET_FIELD", chunk, offset);
		case OP_DEFINE_FIELD: return constant_instruction(manager, "OP_DEFINE_FIELD", chunk, offset);
		case OP_DEFINE_METHOD: return constant_instruction(manager, "OP_DEFINE_METHOD", chunk, offset);
		case OP_DEFINE_STATIC_FIELD: return constant_instruction(manager, "OP_DEFINE_STATIC_FIELD", chunk, offset);
		case OP_DEFINE_STATIC_METHOD: return constant_instruction(manager, "OP_DEFINE_STATIC_METHOD", chunk, offset);
		case OP_INVOKE: return constant_instruction(manager, "OP_INVOKE", chunk, offset) + 2;
		case OP_SUPER: return constant_instruction(manager, "OP_SUPER", chunk, offset);
		case OP_CLOSURE: {
			offset++;
			uint8_t constant = chunk->code[offset++];
			printf("%-16s %4d %s\n", "OP_CLOSURE", constant, lit_to_string(manager, chunk->constants.values[constant]));

			LitFunction* function = AS_FUNCTION(chunk->constants.values[constant]);

			for (int j = 0; j < function->upvalue_count; j++) {
				int isLocal = chunk->code[offset++];
				int index = chunk->code[offset++];

				printf("%04d   |                     %s %d\n", offset - 2, isLocal ? "local" : "upvalue", index);
			}

			return offset;
		}
		default: printf("Unknown opcode %i\n", instruction); return offset + 1;
	}
}