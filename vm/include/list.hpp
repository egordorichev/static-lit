#ifndef LIST_HPP
#define LIST_HPP

#include "lit_gc.hpp"

template<class T>
class List {
public:
    List();
    ~List();

    void add(T value);
    void free();
    int get_count() { return count; }
    int get_capacity() { return capacity; }
    T get(int i) { return values[i]; }

private:
    int count;
    int capacity;
    T* values;
};

#endif