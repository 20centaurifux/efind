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
 */
#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

/**
   @param size number of bytes to allocate
   @return pointer to the allocated memory block

   Allocates memory. Aborts on failure.
 */
void *utils_malloc(size_t size);

/**
   @param ptr pointer to memory block to resize
   @param size number of bytes to allocate
   @return pointer to the new memory block

   Resizes a memory block. Aborts on failure.
 */
void *utils_realloc(void *ptr, size_t size);

/**
   @param dst destination string
   @param src string to append
   @param size at most size - 1 bytes are copied
   @return length of concatenated string, retval >= size when truncated

   Appends a string to another.
 */
size_t utils_strlcat(char *dst, const char *src, size_t size);

/**
   @param str string to trim
   @return length of the trimmed string

   Removes whitespace from beginning and end of a string.
 */
size_t utils_trim(char *str);

/**
   @param a first operand
   @param b second operand
   @param dst location to store sum of a and b
   @return true if an overflow occured

   Adds two positive integer values and checks if an overflow occured.
 */
bool utils_int_add_checkoverflow(int a, int b, int *dst);

/**
   @param name executable to find
   @return a new allocated string or NULL

   Searches for an executable testing all directories found in the PATH
   environment variable.
 */
char *utils_whereis(const char *name);

/**
   @param dir directory name
   @param filename a filename
   @param dst location to write combined path to
   @param max_len maximum buffer size
   @return true on success

   Joins a path with a filename.
 */
bool utils_path_join(const char *dir, const char *filename, char *dst, size_t max_len);

#endif

