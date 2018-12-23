/*
 * Operation codes used by VM
 * to perform operations
 *
 * Uses macro OPCODE that should be defined
 * before including this file
 */

// exit()
// Exits the frame without returning a value
OPCODE(EXIT)
// return(return_value)
// Exits the frame returning a single value
OPCODE(RETURN)
// constant(id, res_reg)
// Copies constant from register id to result register
// Id size is 1 byte
OPCODE(CONSTANT)
// constant(res_reg, id)
// Copies constant from register id to result register
// Id size is 2 bytes
OPCODE(CONSTANT_LONG)
// add(res_reg, a_reg, b_reg)
// Adds values from register a and b into result register
OPCODE(ADD)
// subtract(res_reg, a_reg, b_reg)
// Subtracts values from register a and b into result register
OPCODE(SUBTRACT)
// multiply(res_reg, a_reg, b_reg)
// Multiplies values from register a and b into result register
OPCODE(MULTIPLY)
// divide(res_reg, a_reg, b_reg)
// Divides values from register a and b into result register
OPCODE(DIVIDE)
// modulo(res_reg, a_reg, b_reg)
// Dives by modulo values from register a and b into result register
OPCODE(MODULO)
// power(res_reg, a_reg, b_reg)
// Calculates power of register a in stage b into result register
OPCODE(POWER)
// root(res_reg, a_reg, b_reg)
// Calculates root from register a in stage b into result register
OPCODE(ROOT)
// function(res_reg, function_reg)
// Defines a function from function_reg (constant) and puts it into res_reg
// Uses 1 byte as constant id
OPCODE(DEFINE_FUNCTION)
// function(res_reg, function_reg)
// Defines a function from function_reg (constant) and puts it into res_reg
// Uses 2 bytes as constant id
OPCODE(DEFINE_FUNCTION_LONG)
// true(res_reg)
// Puts true value into res_reg
OPCODE(TRUE)
// false(res_reg)
// Puts false value into res_reg
OPCODE(FALSE)
// nil(res_reg)
// Puts nil value into res_reg
OPCODE(NIL)
// not(res_reg, a_reg)
// Performs not operation on a_reg into res_reg
OPCODE(NOT)

/*
OPCODE(STATIC_INIT)
OPCODE(NEGATE)
OPCODE(SUBTRACT)
OPCODE(MULTIPLY)
OPCODE(DIVIDE)
OPCODE(POP)
OPCODE(NOT)
OPCODE(NIL)
OPCODE(TRUE)
OPCODE(FALSE)
OPCODE(EQUAL)
OPCODE(GREATER)
OPCODE(LESS)
OPCODE(GREATER_EQUAL)
OPCODE(LESS_EQUAL)
OPCODE(NOT_EQUAL)
OPCODE(CLOSE_UPVALUE)
OPCODE(DEFINE_GLOBAL)
OPCODE(GET_GLOBAL)
OPCODE(SET_GLOBAL)
OPCODE(GET_LOCAL)
OPCODE(SET_LOCAL)
OPCODE(GET_UPVALUE)
OPCODE(SET_UPVALUE)
OPCODE(JUMP)
OPCODE(JUMP_IF_FALSE)
OPCODE(LOOP)
OPCODE(CLOSURE)
OPCODE(SUBCLASS)
OPCODE(CLASS)
OPCODE(METHOD)
OPCODE(GET_FIELD)
OPCODE(SET_FIELD)
OPCODE(INVOKE)
OPCODE(CALL)
OPCODE(DEFINE_FIELD)
OPCODE(DEFINE_METHOD)
OPCODE(SUPER)
OPCODE(DEFINE_STATIC_FIELD)
OPCODE(DEFINE_STATIC_METHOD)
OPCODE(POWER)
OPCODE(SQUARE)
OPCODE(ROOT)
OPCODE(IS)
OPCODE(MODULO)
OPCODE(FLOOR)*/