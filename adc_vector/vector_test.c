#include "../common/greatest.h"
#include "adc_vector.h"

#define VEC_INIT_NITEMS(vec, n)                                                \
  for (int i = 0; i < (n); i++)                                                \
  adc_vector_push(vec, i)

TEST test_adc_vector_push(void) {
  int *vec = NULL;

  for (int i = 0; i < 20000; i++) {
    adc_vector_push(vec, i);

    // Test initial capacity is fixed to 4.
    if (i < 4)
      ASSERT_EQm("Capacity is hardcoded to 4 if size <= 4", 4,
                 adc_vector_capacity(vec));

    // Test the capacity is continually doubled once we exceed 4 items.
    if (i >= 4 && i < 8)
      ASSERT_EQm("Capacity is doubled to 8 once size > 4", 8,
                 adc_vector_capacity(vec));
    if (i >= 8 && i < 16)
      ASSERT_EQm("Capacity is doubled again to 16 once size > 8", 16,
                 adc_vector_capacity(vec));
    if (i >= 16 && i < 32)
      ASSERT_EQm("Capacity is doubled again to 32 once size > 16", 32,
                 adc_vector_capacity(vec));
    if (i >= 32 && i < 64)
      ASSERT_EQm("Capacity is doubled again to 64 once size > 32", 64,
                 adc_vector_capacity(vec));

    // Test the size is updated.
    ASSERT_EQ(i + 1, (int)adc_vector_size(vec));
    // Test the vector contains the new item.
    ASSERT_EQ(i, vec[i]);
  }

  ASSERT_EQ(32768, adc_vector_capacity(vec));
  ASSERT_EQ(20000, adc_vector_size(vec));

  adc_vector_free(vec);
  PASS();
}

TEST test_adc_vector_pop(void) {
  int *vec = NULL;
  VEC_INIT_NITEMS(vec, 8);

  // Test the items are deleted and returned.
  for (int i = 7; i >= 0; i--)
    ASSERT_EQ(i, adc_vector_pop(vec));

  // Test the capacity remains the same, and the size is 0.
  ASSERT_EQ(8, adc_vector_capacity(vec));
  ASSERT_EQ(0, adc_vector_size(vec));

  adc_vector_free(vec);
  PASS();
}

TEST test_adc_vector_reserve(void) {
  int *vec = NULL;

  // Test the capacity is reserved.
  adc_vector_reserve(vec, 10);
  ASSERT_EQ(10, adc_vector_capacity(vec));

  // Test the vector capacity is not increased.
  VEC_INIT_NITEMS(vec, 10);
  ASSERT_EQ(10, adc_vector_capacity(vec));
  ASSERT_EQ(10, adc_vector_size(vec));

  // Test the vector capacity doubles.
  adc_vector_push(vec, 11);
  ASSERT_EQ(20, adc_vector_capacity(vec));
  ASSERT_EQ(11, adc_vector_size(vec));

  // Test reserving a smaller amount has no effect.
  adc_vector_reserve(vec, 8);
  ASSERT_EQ(20, adc_vector_capacity(vec));
  ASSERT_EQ(11, adc_vector_size(vec));

  adc_vector_free(vec);
  PASS();
}

TEST test_adc_vector_free(void) {
  int *vec = NULL;

  // Test free has no effect on uninitialized vector.
  adc_vector_free(vec);
  ASSERT_EQ(NULL, vec);

  // Test free destroys the vector.
  VEC_INIT_NITEMS(vec, 16);
  adc_vector_free(vec);
  ASSERT_EQ(NULL, vec);

  PASS();
}

TEST test_adc_vector_insert(void) {
  int *vec = NULL;
  int item = 50;

  // Test insert has no effect on uninitialized vector.
  adc_vector_insert(vec, 0, item);
  ASSERT_EQ(NULL, vec);

  // Test insert has no effect for index out of bounds.
  VEC_INIT_NITEMS(vec, 16);
  adc_vector_insert(vec, -1, item);
  adc_vector_insert(vec, 20, item);
  for (int i = 0; i < 16; i++)
    ASSERT_EQ(i, vec[i]);
  adc_vector_free(vec);

  // Test item can be inserted for existing vector.
  VEC_INIT_NITEMS(vec, 16);
  adc_vector_insert(vec, 10, item);
  ASSERT_EQ(item, vec[10]);
  ASSERT_EQ(17, adc_vector_size(vec));
  ASSERT_EQ(32, adc_vector_capacity(vec));
  // Test all items after i are relocated.
  for (int i = 10; i < 16; i++)
    ASSERT_EQ(i, vec[i + 1]);
  adc_vector_free(vec);

  // Test item can be inserted at beginning for existing vector.
  VEC_INIT_NITEMS(vec, 16);
  adc_vector_insert(vec, 0, item);
  ASSERT_EQ(item, vec[0]);
  ASSERT_EQ(17, adc_vector_size(vec));
  ASSERT_EQ(32, adc_vector_capacity(vec));
  // Test all items after i are relocated.
  for (int i = 0; i < 16; i++)
    ASSERT_EQ(i, vec[i + 1]);
  adc_vector_free(vec);

  // Test item can be inserted at end for existing vector.
  VEC_INIT_NITEMS(vec, 16);
  adc_vector_insert(vec, 15, item);
  for (int i = 0; i < 14; i++)
    ASSERT_EQ(i, vec[i]);
  ASSERT_EQ(item, vec[15]);
  ASSERT_EQ(17, adc_vector_size(vec));
  ASSERT_EQ(32, adc_vector_capacity(vec));
  adc_vector_free(vec);

  PASS();
}

TEST test_adc_vector_erase(void) { SKIPm("TODO!"); }

TEST test_adc_vector_erasen(void) { SKIPm("TODO!"); }

TEST test_adc_vector_size(void) {
  int *vec = NULL;

  // Test size is 0 for uninitialized vector.
  ASSERT_EQ(0, adc_vector_size(vec));

  // Test size is correct for existing vector.
  VEC_INIT_NITEMS(vec, 16);
  ASSERT_EQ(16, adc_vector_size(vec));
  adc_vector_free(vec);

  PASS();
}

TEST test_adc_vector_capacity(void) {
  int *vec = NULL;

  // Test capacity is 0 for uninitialized vector.
  ASSERT_EQ(0, adc_vector_capacity(vec));

  // Test capacity is correct for existing vector.
  VEC_INIT_NITEMS(vec, 20);
  ASSERT_EQ(32, adc_vector_capacity(vec));
  adc_vector_free(vec);

  PASS();
}

TEST test_adc_vector_empty(void) {
  int *vec = NULL;

  // Test empty returns true for uninitialized vector.
  ASSERT(adc_vector_empty(vec));

  // Test empty returns false for existing vector with items.
  VEC_INIT_NITEMS(vec, 4);
  ASSERT_FALSE(adc_vector_empty(vec));

  // Test empty returns true for existing vector with no items.
  adc_vector_erasen(vec, 0, 4);
  ASSERT(adc_vector_empty(vec));
  adc_vector_free(vec);

  // Test empty returns true for free'd vector.
  VEC_INIT_NITEMS(vec, 4);
  adc_vector_free(vec);
  ASSERT(adc_vector_empty(vec));

  PASS();
}

SUITE(adc_vector_suite) {
  RUN_TEST(test_adc_vector_push);
  RUN_TEST(test_adc_vector_pop);
  RUN_TEST(test_adc_vector_reserve);
  RUN_TEST(test_adc_vector_free);
  RUN_TEST(test_adc_vector_insert);
  RUN_TEST(test_adc_vector_erase);
  RUN_TEST(test_adc_vector_erasen);
  RUN_TEST(test_adc_vector_size);
  RUN_TEST(test_adc_vector_capacity);
  RUN_TEST(test_adc_vector_empty);
}

GREATEST_MAIN_DEFS();

int main(int argc, char *argv[]) {
  GREATEST_MAIN_BEGIN();

  RUN_SUITE(adc_vector_suite);

  GREATEST_MAIN_END();
}
