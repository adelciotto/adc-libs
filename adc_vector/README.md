adc_vector
========================

Generic dynamic array implementation similar to the C++ std::vector.

All operations except `adc_vector_insert`, `adc_vector_erase` and `adc_vector_erasen` are O(1) amortized. The vector capacity is doubled as needed.

For some of the functions (`adc_vector_erase`, `adc_vector_erasen`, `adc_vector_insert`) bounds checking is performed. This is not a performance optimized implementation.

Credit to Sean T. Barrett and contributors of stb_ds.h. Some of that code is being used in this library.

# Usage

```c
#include "adc_vector.h"

// Declare an empty vector of type T.
T *vec = NULL;

// Functions (all macros).

// Append the item to the end of the vector.
void adc_vector_push(T *vec, T item);

// Delete the last item from the vector. Returns the deleted item.
T adc_vector_pop(T *vec);

// Free the vector.
void adc_vector_free(T *vec);

// Insert the item at the given index, moving the rest of the array over.
void adc_vector_insert(T *vec, int index, T item);

// Delete the item at the given index, moving the rest of the array over.
void adc_vector_erase(T *vec, int index);

// Delete n items starting at the given index, moving the rest of the array over.
void adc_vector_erasen(T *vec, int index, int n);

// Returns the number of items in the vector.
size_t adc_vector_size(T *vec);

// Returns the current memory capacity of the vector.
size_t adc_vector_capacity(T *vec);

// Returns true if the vector is empty.
bool adc_vector_empty(T *vec);
```
