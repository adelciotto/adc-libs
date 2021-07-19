// adc_vector Vector data structure library by Anthony Del Ciotto.
// Implements a dynamic array similar to the C++ std::vector.
//
// All operations except adc_vector_insert, adc_vector_erase and
// adc_vector_erasen are O(1) amortized. The vector capacity is
// doubled as needed.
//
// For some of the functions (adc_vector_erase, adc_vector_erasen,
// adc_vector_insert) some bounds checking is performed. So this is not a
// performance optimized implementation.
//
// Credit to Sean T. Barrett and contributors of stb_ds.h. Some of that code is
// being used in this library.
//
// MIT License

// Copyright (c) 2021 Anthony Del Ciotto

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef _ADC_VECTOR_H_
#define _ADC_VECTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

// 0.1.0
#define ADC_VECTOR_VERSION_MAJOR 0
#define ADC_VECTOR_VERSION_MINOR 1
#define ADC_VECTOR_VERSION_PATCH 0

#include <stdlib.h> // For realloc, free, size_t
#include <string.h> // For memmove

// adc_vector_push() - Add a new item to the end of the vector.
//
// No return value.
#define adc_vector_push(vec, item)                                             \
  {                                                                            \
    adc__vector_addgrow(vec, 1);                                               \
    (vec)[adc_vector_get_header(vec)->size++] = (item);                        \
  }

// adc_vector_pop() - Delete and return item from the end of the vector.
// Returns the deleted value.
#define adc_vector_pop(vec)                                                    \
  (adc_vector_get_header(vec)->size--, (vec)[adc_vector_get_header(vec)->size])

// adc_vector_reserve() - Requests that the vector capacity be at least enough
// to contain n items.
//
// If n > capacity, the function causes the container to reallocate its storage
// increasing its capacity to n (or greater).
//
// In all other cases, the function call does not cause a reallocation and the
// vector capacity is not affected.
//
// No return value.
#define adc_vector_reserve(vec, n)                                             \
  ((vec) = adc__vector_grow(vec, sizeof(*(vec)), 0, n), (void)0)

// adc_vector_free() - Free the vector.
//
// No return value.
#define adc_vector_free(vec)                                                   \
  {                                                                            \
    if (vec) {                                                                 \
      free(adc_vector_get_header(vec));                                        \
      (vec) = NULL;                                                            \
    }                                                                          \
  }

// adc_vector_insert() - Add a new item at index i.
// Causes the vector to relocate all the items after i, so is a more inefficient
// operation.
//
// If i < 0, no action is taken.
// If i >= size, no action is taken.
//
// No return value.
#define adc_vector_insert(vec, i, item)                                        \
  {                                                                            \
    if ((vec) && ((i) >= 0 && (i) < adc_vector_get_header(vec)->size)) {       \
      adc__vector_addgrow(vec, 1);                                             \
      adc_vector_header *header = adc_vector_get_header(vec);                  \
      header->size++;                                                          \
      size_t num = sizeof(*(vec)) * header->size - 1 - (i);                    \
      memmove(&(vec)[(i) + 1], &(vec)[(i)], num);                              \
      (vec)[i] = (item);                                                       \
    }                                                                          \
  }

// adc_vector_erase() - Delete item at index i.
// Causes the vector to relocate all the items after i, so is a more inefficient
// operation.
//
// No return value.
#define adc_vector_erase(vec, i) adc_vector_erasen(vec, i, 0)

// adc_vector_erasen() - Delete n item at index i.
// Causes the vector to relocate all the items after i, so is a more inefficient
// operation.
//
// If i < 0, no action is taken.
// if i >= size, no action is taken.
// If n is < 0 ,no action is taken.
// If i+n >= size, then all items at position >= i will be deleted.
//
// No return value.
#define adc_vector_erasen(vec, i, n)                                           \
  {                                                                            \
    if ((vec) && ((i) >= 0 && (i) < adc_vector_get_header(vec)->size) &&       \
        (n) >= 0) {                                                            \
      adc_vector_header *header = adc_vector_get_header(vec);                  \
      int maxn = (n);                                                          \
      if ((size_t)((i) + maxn) >= header->size)                                \
        maxn = header->size - (i);                                             \
      size_t num = sizeof(*(vec)) * header->size - maxn - (i);                 \
      memmove(&(vec)[(i)], &(vec)[(i) + maxn], num);                           \
      header->size -= maxn;                                                    \
    }                                                                          \
  }

// adc_vector_size() - Return the number of items in the vector.
//
// Returns 0 for a NULL vector.
#define adc_vector_size(vec) ((vec) ? adc_vector_get_header(vec)->size : 0)

// adc_vector_capacity() - Return the current memory capacity of the vector.
//
// Return 0 for a NULL vector.
#define adc_vector_capacity(vec)                                               \
  ((vec) ? adc_vector_get_header(vec)->capacity : 0)

// adc_vector_empty() - Return true if the vector is empty.
//
// Returns true for a NULL vector.
#define adc_vector_empty(vec) (adc_vector_size(vec) == 0)

typedef struct {
  size_t size;
  size_t capacity;
} adc_vector_header;

#define adc_vector_get_header(vec) ((adc_vector_header *)(vec)-1)

#define adc__vector_addgrow(vec, n)                                            \
  {                                                                            \
    adc_vector_header *header = adc_vector_get_header(vec);                    \
    if (!(vec) || (header->size + (n) > header->capacity)) {                   \
      (vec) = adc__vector_grow(vec, sizeof(*(vec)), (n), 0);                   \
    }                                                                          \
  }

// adc__vector_grow() - Grow the vector capacity by the specified minimum
// amount. If min_cap is 0 then it will be calculated by the function.
static void *adc__vector_grow(void *vec, size_t elemsize, size_t addsize,
                              size_t min_cap) {
  size_t min_size = adc_vector_size(vec) + addsize;

  // Ensure minimum capacity meets required size.
  if (min_size > min_cap)
    min_cap = min_size;

  size_t capacity = adc_vector_capacity(vec);

  // No need to allocate if minimum capacity is less the current capacity.
  if (min_cap <= capacity)
    return vec;

  // Grow the vector by 2*capacity for amortized O(1) operations.
  if (min_cap < (2 * capacity))
    min_cap = 2 * capacity;
  else if (min_cap < 4)
    min_cap = 4;
  size_t newsize = elemsize * min_cap + sizeof(adc_vector_header);
  void *newvec = realloc(vec ? adc_vector_get_header(vec) : 0, newsize);

  // Ensure it points to the vector data.
  newvec = (char *)newvec + sizeof(adc_vector_header);

  if (vec == NULL)
    adc_vector_get_header(newvec)->size = 0;

  adc_vector_get_header(newvec)->capacity = min_cap;
  return newvec;
}

#ifdef __cplusplus
}
#endif

#endif // _ADC_VECTOR_H_
