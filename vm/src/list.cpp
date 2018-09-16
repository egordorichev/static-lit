#include "list.hpp"
#include "lit_common.hpp"

template<typename T>
List<T>::List() {
  count = 0;
  capacity = 0;
  values = nullptr;
}

template<typename T>
List<T>::~List() {
  free();
}

template<typename T>
void List<T>::free() {
  FREE_ARRAY(T, values, capacity);

  count = 0;
  capacity = 0;
  values = nullptr;
}

template<typename T>
void List<T>::add(T value) {
  if (capacity < count + 1) {
    int oldCapacity = capacity;

    capacity = GROW_CAPACITY(oldCapacity);
    values = GROW_ARRAY(values, T, oldCapacity, capacity);
  }

  values[count] = value;
  count++;
}