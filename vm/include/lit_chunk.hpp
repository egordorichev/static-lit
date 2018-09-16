#ifndef LIT_CHUNK_HPP
#define LIT_CHUNK_HPP

#include "list.hpp"
#include "lit_value.hpp"

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_ARRAY(previous, type, oldCount, count) (type *) reallocate((type *) previous, sizeof(type) * (oldCount), sizeof(type) * (count))
#define FREE_ARRAY(type, pointer, oldCount) reallocate(pointer, sizeof(type) * (oldCount), 0)

void* reallocate(void* previous, size_t old_size, size_t new_size);

// Operation codes
enum LitOpCode {
    OP_CONSTANT,
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_RETURN
};

class LitChunk {
private:
    int count;
    int capacity;
    int* lines;
    uint8_t* code;
    List<LitValue> constants;

public:
    LitChunk();
    ~LitChunk();

    void write(uint8_t cd, int line);
    int add_constant(LitValue value);
    int get_line(int i) { return lines[i]; }
    int get_count() { return count; }
    int get_capacity() { return capacity; }
    uint8_t* get_code() { return code; }
    List<LitValue>* get_constants() { return &constants; }
};

#endif