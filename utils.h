/***************************************************************************
    begin........: April 2015
    copyright....: Sebastian Fedrau
    email........: sebastian.fedrau@gmail.com
 ***************************************************************************/

/***************************************************************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License v3 as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License v3 for more details.
 ***************************************************************************/
/**
   @file utils.h
   @brief Utility functions.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
   @version 0.1.0
*/
#include <stdlib.h>

#ifndef UTILS_H
#define UTILS_H

/**
   @param size number of bytes to allocate
   @return pointer to the allocated memory block

   Allocates memory and abort on failure.
 */
void *utils_malloc(size_t size);

/**
   @param ptr pointer to memory block to resize
   @param size number of bytes to allocate
   @return pointer to the new memory block

   Resizes a memory block.
 */
void *utils_realloc(void *ptr, size_t size);

/**
   @param n a number
   @return a power of two

   Finds the smallest power of two that's greater or equal to n.
 */
size_t utils_next_pow2(size_t n);

#endif

