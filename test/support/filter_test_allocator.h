#ifndef FILTER_TEST_ALLOCATOR_H
#define FILTER_TEST_ALLOCATOR_H

#include <stdbool.h>
#include <stddef.h>

void *filter_test_malloc(size_t size);
void filter_test_free(void *pointer);
void allocator_reset(void);
void allocator_fail_next(void);
bool allocator_guards_ok(void);
size_t allocator_live_blocks(void);
size_t allocator_alloc_calls(void);
size_t allocator_free_calls(void);
size_t allocator_errors(void);

#endif
