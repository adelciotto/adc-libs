#include "../common/greatest.h"
#include "adc_vector.h"

#define ASSERT_EQ_INT(expected, actual) ASSERT_EQ_FMT(expected, actual, "%d")

#define ASSERT_EQ_INTm(msg, expected, actual)                                  \
  ASSERT_EQ_FMTm(msg, expected, actual, "%d")

#define VEC_INIT_NITEMS(vec, n)                                                \
  for (int i = 0; i < (n); i++)                                                \
  adc_vector_push(vec, i)

TEST test_adc_vector_push(void) {
  int *vec = NULL;

  for (int i = 0; i < 20000; i++) {
    adc_vector_push(vec, i);

    // Test size is 0 for uninitialized vector.
    if (i < 4)
      ASSERT_EQ_INTm("Capacity is fixed to 4 if size <= 4", 4,
                     (int)adc_vector_capacity(vec));

    // Test size is 0 for uninitialized vector.
    if (i >= 4 && i < 8)
      ASSERT_EQ_INTm("Capacity is doubled to 8 once size > 4", 8,
                     (int)adc_vector_capacity(vec));
    if (i >= 8 && i < 16)
      ASSERT_EQ_INTm("Capacity is doubled again to 16 once size > 8", 16,
                     (int)adc_vector_capacity(vec));
    if (i >= 16 && i < 32)
      ASSERT_EQ_INTm("Capacity is doubled again to 32 once size > 16", 32,
                     (int)adc_vector_capacity(vec));
    if (i >= 32 && i < 64)
      ASSERT_EQ_INTm("Capacity is doubled again to 64 once size > 32", 64,
                     (int)adc_vector_capacity(vec));

    // Test size is incremented.
    ASSERT_EQ_INTm("Size is incremented", i + 1, (int)adc_vector_size(vec));
    // Test item is pushed to end.
    ASSERT_EQ_INTm("Item is added to end", i, vec[i]);
  }

  // Test final capacity and size are correct.
  ASSERT_EQ_INT(32768, (int)adc_vector_capacity(vec));
  ASSERT_EQ_INT(20000, (int)adc_vector_size(vec));

  adc_vector_free(vec);
  PASS();
}

TEST test_adc_vector_pop(void) {
  int *vec = NULL;
  VEC_INIT_NITEMS(vec, 8);

  // Test items are returned from pop.
  for (int i = 7; i >= 0; i--)
    ASSERT_EQ_INTm("Item is returned", i, adc_vector_pop(vec));

  // Test all items are deleted, and capacity is not affected.
  ASSERT_EQ_INTm("Capacity does not change", 8, (int)adc_vector_capacity(vec));
  ASSERT_EQ_INTm("Items are deleted", 0, (int)adc_vector_size(vec));

  adc_vector_free(vec);
  PASS();
}

TEST test_adc_vector_reserve(void) {
  int *vec = NULL;

  // Test capacity is reserved.
  adc_vector_reserve(vec, 10);
  ASSERT_EQ_INTm("Capacity is set to reserved amount", 10,
                 (int)adc_vector_capacity(vec));

  // Test capacity is not affected if size <= reserved.
  VEC_INIT_NITEMS(vec, 10);
  ASSERT_EQ_INTm("Capacity not grown if size <= reserved", 10,
                 (int)adc_vector_capacity(vec));
  ASSERT_EQ_INT(10, (int)adc_vector_size(vec));

  // Test capacity doubles if size > reserved.
  adc_vector_push(vec, 11);
  ASSERT_EQ_INTm("Capacity doubles if size > reserved", 20,
                 (int)adc_vector_capacity(vec));
  ASSERT_EQ_INT(11, (int)adc_vector_size(vec));

  // Test capacity is not affected is smaller amount reserved.
  adc_vector_reserve(vec, 8);
  ASSERT_EQ_INTm("Capacity not affected", 20, (int)adc_vector_capacity(vec));
  ASSERT_EQ_INT(11, (int)adc_vector_size(vec));

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
    ASSERT_EQ_INT(i, vec[i]);
  adc_vector_free(vec);

  // Test item can be inserted for existing vector.
  VEC_INIT_NITEMS(vec, 16);
  adc_vector_insert(vec, 10, item);
  ASSERT_EQ_INTm("Item is inserted", item, vec[10]);
  ASSERT_EQ_INTm("Size is incremented", 17, (int)adc_vector_size(vec));
  ASSERT_EQ_INTm("Capacity is doubled", 32, (int)adc_vector_capacity(vec));
  // Test all items after i are relocated.
  for (int i = 10; i < 16; i++)
    ASSERT_EQ_INTm("Item is relocated", i, vec[i + 1]);
  adc_vector_free(vec);

  // Test item can be inserted at beginning for existing vector.
  VEC_INIT_NITEMS(vec, 16);
  adc_vector_insert(vec, 0, item);
  ASSERT_EQ_INTm("Item is inserted at beginning", item, vec[0]);
  ASSERT_EQ_INTm("Size is incremented", 17, (int)adc_vector_size(vec));
  ASSERT_EQ_INTm("Capacity is doubled", 32, (int)adc_vector_capacity(vec));
  // Test all items after i are relocated.
  for (int i = 0; i < 16; i++)
    ASSERT_EQ_INTm("Item is relocated", i, vec[i + 1]);
  adc_vector_free(vec);

  // Test item can be inserted at end for existing vector.
  VEC_INIT_NITEMS(vec, 16);
  adc_vector_insert(vec, 15, item);
  for (int i = 0; i < 14; i++)
    ASSERT_EQ_INT(i, vec[i]);
  ASSERT_EQ_INTm("Item is inserted at end", item, vec[15]);
  ASSERT_EQ_INTm("Size is incremented", 17, (int)adc_vector_size(vec));
  ASSERT_EQ_INTm("Capacity is doubled", 32, (int)adc_vector_capacity(vec));
  adc_vector_free(vec);

  PASS();
}

TEST test_adc_vector_erasen(void) {
  int *vec = NULL;

  // Test erasen has no affect for uninitialized vector.
  adc_vector_erasen(vec, 0, 3);
  ASSERT_EQ(NULL, vec);

  VEC_INIT_NITEMS(vec, 6);

  // Test erasen has no affect for i < 0.
  adc_vector_erasen(vec, -1, 3);
  ASSERT_EQ_INTm("Size not affected", 6, (int)adc_vector_size(vec));
  ASSERT_EQ_INTm("Capacity not affected", 8, (int)adc_vector_capacity(vec));
  for (int i = 0; i < 6; i++)
    ASSERT_EQ_INT(i, vec[i]);

  // Test erasen has no affect for i >= size.
  adc_vector_erasen(vec, 6, 3);
  ASSERT_EQ_INTm("Size not affected", 6, (int)adc_vector_size(vec));
  ASSERT_EQ_INTm("Capacity not affected", 8, (int)adc_vector_capacity(vec));
  for (int i = 0; i < 6; i++)
    ASSERT_EQ_INT(i, vec[i]);

  // Test erasen has no effect for n < 0.
  adc_vector_erasen(vec, 0, -3);
  ASSERT_EQ_INTm("Size not affected", 6, (int)adc_vector_size(vec));
  ASSERT_EQ_INTm("Capacity not affected", 8, (int)adc_vector_capacity(vec));
  for (int i = 0; i < 6; i++)
    ASSERT_EQ_INT(i, vec[i]);

  // Test erasen deletes n items at position i.
  adc_vector_erasen(vec, 1, 3);
  ASSERT_EQ_INTm("Size decremented", 3, (int)adc_vector_size(vec));
  ASSERT_EQ_INTm("Capacity not affected", 8, (int)adc_vector_capacity(vec));
  ASSERT_EQ_INT(0, vec[0]);
  ASSERT_EQ_INT(4, vec[1]);
  ASSERT_EQ_INT(5, vec[2]);

  adc_vector_free(vec);

  // Test n is capped to end of vector.
  VEC_INIT_NITEMS(vec, 6);

  adc_vector_erasen(vec, 1, 6);
  ASSERT_EQ_INTm("Size decremented", 1, (int)adc_vector_size(vec));
  ASSERT_EQ_INTm("Capacity not affected", 8, (int)adc_vector_capacity(vec));
  ASSERT_EQ_INT(0, vec[0]);

  adc_vector_erasen(vec, 0, 50);
  ASSERT_EQ_INTm("Size decremented", 0, (int)adc_vector_size(vec));
  adc_vector_free(vec);

  PASS();
}

TEST test_adc_vector_size(void) {
  int *vec = NULL;

  // Test size is 0 for uninitialized vector.
  ASSERT_EQ_INTm("Returns 0 for uninitialized vector", 0,
                 (int)adc_vector_size(vec));

  // Test size is correct for existing vector.
  VEC_INIT_NITEMS(vec, 16);
  ASSERT_EQ_INT(16, (int)adc_vector_size(vec));
  adc_vector_free(vec);

  PASS();
}

TEST test_adc_vector_capacity(void) {
  int *vec = NULL;

  // Test capacity is 0 for uninitialized vector.
  ASSERT_EQ_INTm("Returns 0 for uninitialized vector", 0,
                 (int)adc_vector_capacity(vec));

  // Test capacity is correct for existing vector.
  VEC_INIT_NITEMS(vec, 20);
  ASSERT_EQ_INT(32, (int)adc_vector_capacity(vec));
  adc_vector_free(vec);

  PASS();
}

TEST test_adc_vector_empty(void) {
  int *vec = NULL;

  // Test empty returns true for uninitialized vector.
  ASSERTm("Returns true for uninitialized vector", adc_vector_empty(vec));

  // Test empty returns false for existing vector with items.
  VEC_INIT_NITEMS(vec, 4);
  ASSERT_FALSEm("Returns false for vector with items", adc_vector_empty(vec));

  // Test empty returns true for existing vector with no items.
  adc_vector_erasen(vec, 0, 4);
  ASSERTm("Returns true for vector with no items", adc_vector_empty(vec));
  adc_vector_free(vec);

  // Test empty returns true for free'd vector.
  VEC_INIT_NITEMS(vec, 4);
  adc_vector_free(vec);
  ASSERTm("Returns true for free'd vector", adc_vector_empty(vec));

  PASS();
}

SUITE(adc_vector_suite) {
  RUN_TEST(test_adc_vector_push);
  RUN_TEST(test_adc_vector_pop);
  RUN_TEST(test_adc_vector_reserve);
  RUN_TEST(test_adc_vector_free);
  RUN_TEST(test_adc_vector_insert);
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
