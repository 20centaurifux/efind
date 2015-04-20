#include <stdlib.h>

#ifndef UTILS_H
#define UTILS_H

void *utils_malloc(size_t size);
void *utils_realloc(void *ptr, size_t size);
void utils_free(void *ptr);
size_t utils_next_pow2(size_t n);

#endif
