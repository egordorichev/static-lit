#include "lit_common.hpp"

void* reallocate(void* previous, size_t old_size, size_t new_size) {
  if (new_size == 0) {
    free(previous);

    return nullptr;
  }

  return realloc(previous, new_size);
}
