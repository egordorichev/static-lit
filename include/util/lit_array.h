#ifndef LIT_ARRAY_H
#define LIT_ARRAY_H

#define DECLARE_ARRAY(name, type, shr) \
	typedef struct { \
		int capacity; \
		int count; \
		type* values; \
	} name; \
	void lit_init_##shr(name* array); \
	void lit_free_##shr(LitVm* vm, name* array); \
	void lit_##shr##_write(LitVm* vm, name* array, type value);

#define DEFINE_ARRAY(name, type, shr) \
	void lit_init_##shr(name* array) { \
		array->values = NULL; \
		array->capacity = 0; \
		array->count = 0; \
	} \
	\
	void lit_free_##shr(LitVm* vm, name* array) { \
		FREE_ARRAY(vm, type, array->values, array->capacity); \
		lit_init_##shr(array); \
	} \
	\
	void lit_##shr##_write(LitVm* vm, name* array, type value) { \
		if (array->capacity < array->count + 1) { \
			int old_capacity = array->capacity; \
			array->capacity = GROW_CAPACITY(old_capacity); \
			array->values = GROW_ARRAY(vm, array->values, type, old_capacity, array->capacity); \
		} \
		\
		array->values[array->count] = value; \
		array->count++; \
	}

#endif